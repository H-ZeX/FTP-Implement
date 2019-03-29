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
 *
 * use bool is ok, for that the sig_atomic_t has such comment
 * C99: An integer type that can be accessed as an atomic entity,
 * even in the presence of asynchronous interrupts.
 * It is not currently necessary for this to be machine-specific.
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
     * At any time, only one thread can handle the same session,
     * Should NOT handle it in multiple threads.
     * How this class make sure it?
     * 1. User's cmd fd MUST be EPOLLONESHOT,
     * to make sure when one session is handled,
     * epoll_wait will not return this fd.
     * 2. Because when an user's cmd fd is available,
     * its session is get by from userRecord,
     * which is a map from fd to session* .
     * So at any time when the session alive,
     * the corresponding fd should NOT closed,
     * which will avoid the OS reuse this fd.
     * 3. When destroy session and close the fd, it lock the userRecordMutex,
     * then even if the os reuse the fd before finish destroy session,
     * the openNewSession method can NOT create new Session
     * and write to the userRecord map, and can NOT begin to handle the fd.
     * 4.The openNewSession method and epoll_wait is run in the same thread,
     * so before the openNewSession finished, the user's fd will not be used.
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
                            size_t threadCntOfThreadPool = DEFAULT_THREAD_CNT_OF_THREAD_POOL) {
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
    // run on control thread
    bool startAndRun() {
        const string mainPort = to_string(cmdListenPort);
        int mainFd = openListenFd(mainPort.c_str(), FTP_CMD_BACKLOG);
        if (mainFd < 0) {
            warning(("FTP::startAndRun open " + mainPort + " port failed").c_str());
            return false;
        }
        // mutexLock(epollMutex);

        // epoll_create(size)'s size is deprecated since linux 2.6.8,
        // but must be positive
        if ((epollFd = epoll_create(1)) < 0) {
            warningWithErrno("FTP::startAndRun epoll_create", errno);
            // mutexUnlock(epollMutex);
            return false;
        }
        if (!addToEpollInterestList(mainFd, epollFd)) {
            warningWithErrno("FTP::startAndRun addToEpollInterestList failed", errno);
            // mutexUnlock(epollMutex);
            return false;
        }
        // mutexUnlock(epollMutex);
        signalWrap(SIGINT, FTP::signalHandler);
        signalWrap(SIGPIPE, FTP::signalHandler);
        this->pool->start();
        eventLoop(mainFd);
        // mainFd is listenFd, so we can close it here
        closeFileDescriptor(mainFd);
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
        // Can only close epollFd here.
        // Because destroySession uses epollFd.
        closeFileDescriptor(epollFd);
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
        this->pool = ThreadPool::getInstance(threadCntOfThreadPool,
                                             sigToBlock);
    }

    /*
     * run on control thread
     */
    void eventLoop(int mainFd) {
        const int evArraySize = FTP_MAX_USER_ONLINE_CNT;
        auto *const evArray = new epoll_event[evArraySize];
        const int sigSet[] = {SIGINT, 0};
        sigset_t sigToBlock{}, oldSigSet{};
        makeSigSet(sigSet, sigToBlock);
        while (true) {

            // make sure there is no race condition:
            // the signal occur after check willExit and before epoll_wait
            // then the epoll_wait may not wake up.
            pthreadSigmaskWrap(SIG_BLOCK, &sigToBlock, &oldSigSet);
            if (willExit) {
                break;
            }
            int waitFdCnt = epollPWaitWrap(this->epollFd, evArray, evArraySize, -1, oldSigSet);
            pthreadSigmaskWrap(SIG_SETMASK, &oldSigSet);

            for (int i = 0; i < waitFdCnt && !willExit; i++) {
                // all these code will not block infinitely,
                // so needn't to block SIGINT.
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

        if (!sendHelloMsg(fd)) {
            warning(("FTP::openNewSession failed, fd: " + to_string(fd)).c_str());
            errorHandler(fd);
        } else {
            // mutexLock(epollMutex);
            // MUST has EPOLLONESHOT here
            bool ok = addToEpollInterestList(fd, epollFd, EPOLLIN | EPOLLONESHOT);
            // mutexUnlock(epollMutex);
            if (!ok) {
                warning(("FTP::openNewSession failed, fd: " + to_string(fd)).c_str());
                errorHandler(fd);
            } else {
                myLog(("accept one user, fd: " + to_string(fd)).c_str());
            }
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

    /*
     * run one threadPool's work thread
     */
    static void *userHandler(void *argv) {
        Session &session = *(Session *) argv;
        session.handle();
        return nullptr;
    }

    /*
     * run one threadPool's work thread
     */
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

    /*
     * should only be call with valid param.
     * for that can't code to check whether it is valid.
     *
     * run one threadPool's work thread
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

        epoll_event ev{};
        // MUST has EPOLLONESHOT here
        ev.events = EPOLLIN | EPOLLONESHOT;
        ev.data.fd = fd;

        // mutexLock(owner.epollMutex);
        bool ok = epollCtlWrap(owner.epollFd, EPOLL_CTL_MOD, fd, &ev);
        // mutexUnlock(owner.epollMutex);
        if (!ok) {
            owner.errorHandler(fd);
        }
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
     * run on multiple threads.
     */
    void destroySession(int fd) {
        /*
         * MUST make sure that, while destroySession,
         * the session associate with the fd is not invoked
         * (no matter the session is new session or origin session).
         *
         * After close the fd, the OS may reuse the fd,
         * however, the openNewSession will block because of userRecordMutex
         * so no new session will be created.
         *
         * Make sure epoll_ctl_del the fd from epoll interest list before close it.
         * To avoid the epoll_wait return this fd.
         *
         * For that the user's cmd FD is EPOLLONESHOT,
         * so we can make sure that while destroy the session,
         * no other thread will invoke this session.
         */
        mutexLock(userRecordMutex);
        auto user = userRecord.find(fd);
        if (user == userRecord.end()) {
            bug("FTP::destroySession failed: the fd has no corresponding session");
        }
        delete user->second;
        userRecord.erase(user);

        /*
         * MUST make sure epoll_ctl_del before close the fd.
         * See the epoll_wait manual for what will happen when close an fd
         * which is monitored by epoll_wait.
         *
         * User epollMutex to make sure the epoll_ctl_del run before closeFileDescriptor.
         * TODO will this way work?
         */
        mutexLock(epollMutex);
        if (!epollCtlWrap(epollFd, EPOLL_CTL_DEL, fd)) {
            bugWithErrno("FTP::destroySession EPOLL_CTL_DEL failed", errno, true);
        }
        mutexUnlock(epollMutex);

        closeFileDescriptor(fd);
        mutexUnlock(userRecordMutex);

        mutexLock(userOnlineCntMutex);
        userOnlineCnt -= 1;
        mutexUnlock(userOnlineCntMutex);
        myLog(("destroy fd: " + to_string(fd)).c_str());
    }

    /*
     * This method run when SIGINT and SIGPIPE occur.
     * So it can ONLY invoke async-signal-safe functions (read the CSAPP P767)
     */
    static void signalHandler(int num) {
        errno_t tmp = errno;
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
        errno = tmp;
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
     * The userRecord is protected by userRecordMutex.
     * when operate userRecord, MUST lock this mutex,
     * no matter whether read or write.
     * Lock while read is to make the lock act as a memory barrier.
     */
    pthread_mutex_t userRecordMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t epollMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t userOnlineCntMutex = PTHREAD_MUTEX_INITIALIZER;

    /*
     * map user's cmdFd to user's Session*
     *
     * protected by userRecordMutex
     */
    unordered_map<int, Session *> userRecord;

    /*
     * protected by userOnlineCntMutex
     */
    int userOnlineCnt = 0;

    /*
     * use atomic for memory barrier
     */
    atomic_int epollFd = -1;

private:
    /*
     * cmdListenPort is ONLY used in control thread
     */
    const int cmdListenPort = -1;
    /*
     * this pool var is ONLY used on control thread
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
