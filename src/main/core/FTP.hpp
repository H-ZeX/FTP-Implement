#ifndef __FTP_H__
#define __FTP_H__

#include "src/main/util/Def.hpp"
#include "src/main/util/NetUtility.hpp"
#include "src/main/tools/ThreadPool.hpp"
#include "src/main/util/Utility.hpp"
#include "Session.hpp"
#include "src/main/config/config.hpp"

#include <csignal>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>

class FTP;

/*
 * this var is operate by signal handler
 * so it must be global var
 */
static volatile bool willExit = false;

/*
 * The ftpInstanceMutex acts as ftpInstance's memory barrier.
 *
 * For that pthread's work function should be static,
 * so I need to pass the FTP instance to them.
 * I can pass the instance through the argv, or through global var.
 * However, I don't whether another thread will see the complete object
 * constructed in another thread. So I need an mutex to act as memory barrier.
 * So it will be unnecessary to pass the instance through argv.
 */
static pthread_mutex_t ftpInstanceMutex = PTHREAD_MUTEX_INITIALIZER;
static FTP *ftpInstance = nullptr;

/**
 * @warning
 * Thread-Safety: NOT MT-safe !
 * @note
 * Because this class use some global var, so it must be singleton.
 */
class FTP {
    /*
     * The user's cmd fd MUST be EPOLLONESHOT,
     * or this class's state will be destroy because that multiple thread handle the data.
     *
     * In order to avoid the OS reuse the fd,
     * which may make multiple thread handler same data of userRecord,
     * so the close(fd) can ONLY be called at destroySession,
     */

    // TODO is epoll functions thread safe ?
public:
    /**
     * @note
     * Should delete the instance after finishing using it.
     * @warning
     * MUST ONLY delete it one time
     * @return
     */
    static FTP *getInstance(int cmdListenPort = 21,
                            int threadCntOfThreadPool = DEFAULT_THREAD_CNT_OF_THREAD_POOL) {
        // the mutex act as memory barrier
        mutexLock(ftpInstanceMutex);
        if (ftpInstance == nullptr) {
            ftpInstance = new FTP(cmdListenPort, threadCntOfThreadPool);
        }
        mutexUnlock(ftpInstanceMutex);
        return ftpInstance;
    }

    /**
     * @note
     * This method will block until this FTP server stop.
     * @return whether star the server success.
     */
    bool startAndRun() {
        const string mainPort = to_string(cmdListenPort);
        int mainFd = openListenFd(mainPort.c_str(), FTP_CMD_BACK_LOG);
        if (mainFd < 0) {
            warning(("FTP::startAndRun open " + mainPort + " port failed").c_str());
            return false;
        }
        // the mutex act as the memory barrier for epollFd
        mutexLock(epollMutex);
        // epoll_create(size)'s size is deprecated since linux 2.6.8,
        // but must be positive
        if ((epollFd = epoll_create(1)) < 0) {
            warningWithErrno("FTP::startAndRun epoll_create", errno);
            mutexUnlock(epollMutex);
            return false;
        }
        if (!addToEpollInterestList(mainFd, epollFd)) {
            warningWithErrno("FTP::startAndRun addToEpollInterestList failed", errno);
            mutexUnlock(epollMutex);
            return false;
        }
        mutexUnlock(epollMutex);
        signalWrap(SIGINT, FTP::signalHandler);
        signalWrap(SIGPIPE, FTP::signalHandler);
        this->pool->start();
        eventLoop(mainFd);
        closeFileDescriptor(mainFd);
        closeFileDescriptor(epollFd);
        return true;
    }

    ~FTP() {
        // delete the pool will block until all work threads in ThreadPool end.
        delete FTP::pool;
        mutexLock(userRecordMutex);
        for (auto &it : userRecord) {
            assert(it.second != nullptr);
            destroySession(it.second->getCmdFd());
        }
        mutexUnlock(userRecordMutex);
        mutexDestroy(userRecordMutex);
        mutexDestroy(epollMutex);
        mutexDestroy(userOnlineCntMutex);
    }

private:
    explicit FTP(int cmdListenPort, size_t threadCntOfThreadPool)
            : cmdListenPort(cmdListenPort) {
        if (cmdListenPort <= 0) {
            // TODO replace this using exception
            bug("FTP::getInstance failed: cmdListenPort <=0");
        }
        if (threadCntOfThreadPool < 0) {
            threadCntOfThreadPool = DEFAULT_THREAD_CNT_OF_THREAD_POOL;
        }
        this->pool = ThreadPool::getInstance(threadCntOfThreadPool,
                                             sigToBlock);
    }

    void eventLoop(int mainFd) {
        // make sure to delete this array
        auto *const evArray = new epoll_event[FTP_MAX_USER_ONLINE_CNT];
        const int evArraySize = FTP_MAX_USER_ONLINE_CNT;
        while (true) {
            int waitFdCnt = epollWaitWrap(this->epollFd, evArray, evArraySize, -1);
            for (int i = 0; i < waitFdCnt && !willExit; i++) {
                if (evArray[i].data.fd != mainFd) {
                    if (evArray[i].events & EPOLLIN) {
                        this->gotoSessionHandler(evArray[i].data.fd);
                    } else if (evArray[i].events & (EPOLLHUP | EPOLLERR)) {
                        this->destroySession(evArray[i].data.fd);
                    } else {
                        bug("FTP::controlThreadRunnable, has unexpected events");
                    }
                } else {
                    if (evArray[i].events & EPOLLIN) {
                        this->openNewSession(mainFd);
                    } else if (evArray[i].events & (EPOLLHUP | EPOLLERR)) {
                        bug("FTP::controlThreadRunnable mainFd closed exceptionally");
                    } else {
                        bug("FTP::controlThreadRunnable, has unexpected events");
                    }
                }
            }
            if (willExit) {
                break;
            }
        }
        delete[] evArray;
    }

    /*
     * run on control thread
     */
    void openNewSession(int mainFd) {
        // If don't accept, then can this connection be accepted again?
        // (will epoll_wait return again and use accept to get this connection).
        int fd = acceptConnect(mainFd);
        if (fd < 0) {
            return;
        }
        bool canAddNewSession = true;
        mutexLock(userOnlineCntMutex);
        if (userOnlineCnt >= FTP_MAX_USER_ONLINE_CNT) {
            canAddNewSession = false;
        } else {
            userOnlineCnt++;
        }
        mutexUnlock(userOnlineCntMutex);
        if (!canAddNewSession) {
            sendFailedMsg(fd);
            closeFileDescriptor(fd);
            warning(("FTP::openNewSession failed, fd: " + to_string(fd)).c_str());
            return;
        }

        // Include the `new` statement in the mutex block to
        // make sure other thread read a complete constructed session object.
        mutexLock(userRecordMutex);
        // TODO refine the OOM policy
        // If out of memory, there will be an exception here.
        auto *session = new Session(fd, callbackOnEndOfCmd, callbackOnEndOfSession, this->thisMachineIP);
        assert(userRecord.find(fd) == userRecord.end());
        userRecord[fd] = session;
        mutexUnlock(userRecordMutex);
        // MUST has EPOLLONESHOT here
        if (!sendHelloMsg(fd)
            || !addToEpollInterestList(fd, epollFd, EPOLLIN | EPOLLONESHOT)) {
            warning(("FTP::openNewSession failed, fd: " + to_string(fd)).c_str());
            errorHandler(fd);
        } else {
            myLog(("accept one user, fd: " + to_string(fd)).c_str());
        }
    }

    /*
     * run on control thread
     */
    void gotoSessionHandler(int fd) {
        // although just read the userRecord here,
        // the lock is necessary to act as memory barrier.
        mutexLock(userRecordMutex);
        auto thisUser = userRecord.find(fd);
        assert(thisUser != userRecord.end());
        mutexUnlock(userRecordMutex);

        if (!pool->addTask(Task(userHandler, thisUser->second))) {
            errorHandler(fd);
            warning("FTP gotoSessionHandler: one session be closed because of the full task queue");
        }
    }

    /*
     * The callbackOnEndOfCmd, callbackOnEndOfSession, userHandler
     * is called in the same thread: one of the ThreadPool's work thread.
     * The ThreadPool grantee that the argv will be safely published to work thread
     * (see the ThreadPool::addTask).
     */

    static void *userHandler(void *argv) {
        Session &session = *(Session *) argv;
        session.handle();
        return nullptr;
    }

    static void callbackOnEndOfSession(void *argv) {
        Session &session = *(Session *) argv;
        const int fd = session.getCmdFd();
        assert(fd >= 3);
        // the mutex act as memory barrier
        // TODO if just get the point, will I get the complete state of ftpInstance
        mutexLock(ftpInstanceMutex);
        FTP &owner = *::ftpInstance;
        mutexUnlock(ftpInstanceMutex);
        owner.destroySession(fd);
    }

    /**
     * should only be call with valid param.
     * for that can't code to check whether it is valid.
     */
    static void callbackOnEndOfCmd(void *argv) {
        Session &session = *(Session *) argv;
        const int fd = session.getCmdFd();
        assert(fd >= 3);

        // after EPOLL_CTL_MOD success, epoll_wait may return this fd,
        // so should be careful of the thread safety and memory visibility after EPOLL_CTL_MOD
        mutexLock(ftpInstanceMutex);
        FTP &owner = *::ftpInstance;
        mutexUnlock(ftpInstanceMutex);

        mutexLock(owner.epollMutex);
        epoll_event ev{};
        // MUST has EPOLLONESHOT here
        ev.events = EPOLLIN | EPOLLONESHOT;
        ev.data.fd = fd;
        if (!epollCtlWrap(owner.epollFd, EPOLL_CTL_MOD, fd, &ev)) {
            owner.errorHandler(fd);
        }
        mutexUnlock(owner.epollMutex);
    }

    bool sendFailedMsg(int fd) {
        const char msg[] = "500 Server Internal Error" END_OF_LINE;
        return setNonBlocking(fd) && writeAllData(fd, msg, sizeof(msg) - 1);
    }

    bool sendHelloMsg(int fd) {
        const char msg[] = "220 hzxFTP (0.01)" END_OF_LINE;
        return writeAllData(fd, msg, sizeof(msg) - 1);
    }

    void errorHandler(int fd) {
        sendFailedMsg(fd);
        destroySession(fd);
    }

    /*
     * This method will run in multiple threads.
     */
    void destroySession(int fd) {
        /*
         * MUST make sure that, between the destroy process,
         * the fd is not reuse by OS,
         * so we MUST free the fd after all delete work.
         *
         * For that the user's cmd FD is EPOLLONESHOT,
         * so we can make sure that while destroy the session,
         * no other thread will invoke this session.
         */
        mutexLock(userRecordMutex);
        auto theUser = userRecord.find(fd);
        if (theUser == userRecord.end()) {
            bug("FTP::destroySession failed: the fd has no corresponding session");
        }
        userRecord.erase(theUser);
        delete theUser->second;
        if (!epollCtlWrap(epollFd, EPOLL_CTL_DEL, fd)) {
            bugWithErrno("FTP::destroySession EPOLL_CTL_DEL failed", errno, true);
        }
        closeFileDescriptor(fd);
        mutexUnlock(userRecordMutex);

        mutexLock(userOnlineCntMutex);
        userOnlineCnt -= 1;
        mutexUnlock(userOnlineCntMutex);
        myLog(("destroy fd: " + to_string(fd)).c_str());
    }

    /*
     * This method run when SIGINT occur.
     * So it can ONLY invoke async-signal-safe functions (read the CSAPP P767)
     */
    static void signalHandler(int num) {
        switch (num) {
            case SIGINT: {
                willExit = true;
                const char msg[] = "Exiting...\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                break;
            }
            case SIGPIPE: {
                const char msgPipe[] = "Broken Pipe\n";
                write(STDERR_FILENO, msgPipe, sizeof(msgPipe) - 1);
                break;
            }
            default: {
                const char msgUnexpect[] = "FTP::interruptHandler: call me with unexpected signal";
                write(STDERR_FILENO, msgUnexpect, sizeof(msgUnexpect) - 1);
                _exit(BUG_EXIT);
            }
        }
    }

    /**
     * @return whether success, when fail, the errno is set appropriately,
     * errno can be ENOMEM ENOSPC.
     */
    static bool addToEpollInterestList(int fd, int epollFd, uint32_t event = EPOLLIN) {
        epoll_event ev{};
        ev.events = event;
        ev.data.fd = fd;
        return epollCtlWrap(epollFd, EPOLL_CTL_ADD, fd, &ev);
    }

private:
    /*
     * The var below will be use in multiThreads.
     */

    /*
     * The userRecord is protected by userRecordMutex.
     * when operate userRecord, MUST lock this mutex,
     * no matter whether read or write.
     * Lock while read is to make the lock act as a memory barrier.
     */
    pthread_mutex_t userRecordMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t epollMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t userOnlineCntMutex = PTHREAD_MUTEX_INITIALIZER;

    /*
     * map cmdFd to Session*
     */
    unordered_map<int, Session *> userRecord;

    int userOnlineCnt = 0;
    atomic_int epollFd = -1;

private:
    const int cmdListenPort{};
    constexpr static size_t DEFAULT_THREAD_CNT_OF_THREAD_POOL = 128;
    /*
     * The var below will be use in ONLY on control thread.
     */
    ThreadPool *pool = nullptr;

private:
    /*
     * the 0 indicate the end of sigToBlock
     */
    const int sigToBlock[3] = {SIGPIPE, SIGINT, 0};
    const string thisMachineIP = getThisMachineIp();
};

#endif
