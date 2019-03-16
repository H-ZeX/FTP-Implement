// #ifndef __FTP_H__
// #define __FTP_H__
//
// #include "src/main/util/Def.hpp"
// #include "src/main/util/NetUtility.hpp"
// #include "Session.hpp"
// #include "src/main/tools/ThreadPool.hpp"
// #include "src/main/util/Utility.hpp"
//
// #include <csignal>
// #include <sys/epoll.h>
// #include <sys/resource.h>
// #include <sys/socket.h>
// #include <sys/types.h>
// #include <unistd.h>
// #include <cassert>
//
// /*
//  * this var is operate by signal handler
//  * so it must be global var
//  */
// volatile bool willExit = false;
//
// /**
//  * Thread-Safety: NOT MT-safe !
//  *
//  */
// class FTP {
//     /*
//      * The user's cmd fd MUST be EPOLLONESHOT,
//      * or this class's state will be destroy because that multiple thread handle the data.
//      *
//      * In order to avoid the OS reuse the fd,
//      * which may make multiple thread handler same data of userRecord,
//      * so the endOfSession(fd) operation can ONLY be called at destroySession,
//      */
//
//     // TODO is epoll functions thread safe ?
// public:
//
//     void start(int port = 21, int threadCnt = 128) {
//         this->pool = new ThreadPool(threadCnt, sigToBlock);
//         pthread_t mainTid{};
//         ArgvOfControlThread argv(this, port);
//         if (!createThread(mainTid, controlThreadRunnable, (void *) (&argv))) {
//             warning("create main thread failed, FTP server exist");
//             return;
//         }
//         if (!joinThread(mainTid)) {
//             bugWithErrno("FTP::start join control thread failed", errno, true);
//         }
//     }
//
//     ~FTP() {
//         for (auto &it : userRecord) {
//             if (it.second != nullptr) {
//                 delete (it.second);
//             }
//         }
//         delete FTP::pool;
//         delete FTP::currentFdCnt;
//     }
//
// private:
//     /**
//      * @param param the port to listen
//      */
//     static void *controlThreadRunnable(void *param) {
//         ArgvOfControlThread &argv = *((ArgvOfControlThread *) param);
//         const string mainPort = to_string(argv.mainPort);
//         FTP &owner = *(argv.ftp);
//
//         int mainFd;
//         if ((mainFd = openListenFd(mainPort.c_str())) < 0) {
//             warning(("FTP::controlThreadRunnable open " + mainPort + " port failed").c_str());
//             exit(OPEN_MAINFD_ERROR);
//         }
//         // epoll_create(size)'s size is deprecated since linux 2.6.8,
//         // but must be positive
//         if ((owner.epollFd = epoll_create(1)) < 0) {
//             warningWithErrno("FTP::controlThreadRunnable epoll_create");
//             exit(EPOLL_CREATE_ERROR);
//         }
//         if (addToEpollInterestList(mainFd, owner.epollFd)) {
//             if (errno == ENOMEM) {
//                 warningWithErrno("FTP::callbackOnEndOfCmd EPOLL_CTL_ADD");
//                 exit(EPOLL_ADD_MAINFD_ERROR);
//             } else {
//                 bugWithErrno("FTP::controlThreadRunnable EPOLL_CTL_ADD failed", errno, true);
//             }
//         }
//         atomic_fetch_add<int>(owner.currentFdCnt, 1);
//         assert(owner.currentFdCnt->load() <= MAX_EPOLL_SIZE);
//
//         eventLoop(owner, mainFd);
//         return nullptr;
//     }
//
//     static void eventLoop(FTP &owner, int mainFd) {
//         int waitFdCnt = 0;
//         sigset_t set;
//         epoll_event evArray[MAX_EPOLL_SIZE];
//         while (true) {
//             // epoll_pwait will return when a signal is caught.
//             // this can let us response to SIGINT fast
//             if ((waitFdCnt = epoll_pwait(owner.epollFd, evArray, MAX_EPOLL_SIZE, -1,
//                                          makeSigSetForEpollWait(&set))) < 0 &&
//                 errno != EINTR) {
//                 bugWithErrno("FTP::controlThreadRunnable epoll_wait failed", errno, true);
//             } else if (errno == EINTR && willExit) {
//                 continue;
//             } else if (willExit) {
//                 break;
//             }
//
//             for (int i = 0; i < waitFdCnt; i++) {
//                 if (evArray[i].data.fd != mainFd) {
//                     if (evArray[i].events & EPOLLIN) {
//                         owner.gotoSessionHandler(evArray[i].data.fd);
//                     } else if (evArray[i].events & (EPOLLHUP | EPOLLERR)) {
//                         owner.destroySession(evArray[i].data.fd);
//                     } else {
//                         bug("FTP::controlThreadRunnable, has unexpected events");
//                     }
//                 } else {
//                     if (evArray[i].events & EPOLLIN) {
//                         owner.openNewSession(mainFd);
//                     } else if (evArray[i].events & (EPOLLHUP | EPOLLERR)) {
//                         bug("FTP::controlThreadRunnable mainFd exit exceptional");
//                     } else {
//                         bug("FTP::controlThreadRunnable, has unexpected events");
//                     }
//                 }
//             }
//         }
//     }
//
//     // run on multiple thread
//     static void *userHandler(void *argv) {
//         Session &s = *(Session *) argv;
//         s.handle();
//         return nullptr;
//     }
//
//     static void callbackOnEndOfSession(void *argv) {
//         int fd = ((Session *) argv)->getCmdFd();
//         if (fd < 3) {
//             bug(("FTP::callbackOnEndOfSession " + to_string(fd)).c_str());
//         }
//         destroySession(fd);
//     }
//
//     /**
//      * should only be call with valid param.
//      * for that can't code to check whether it is valid.
//      */
//     static void callbackOnEndOfCmd(void *argv) {
//         /*
//          * run on multiple thread
//          */
//         int fd = ((Session *) argv)->getCmdFd();
//         if (fd < 3) {
//             bug(("FTP::callbackOnEndOfCmd, the fd is illegal, fd: " + to_string(fd)).c_str());
//         }
//
//         // the epoll_ctl will copy ev to kernel space, so we needn't to keep ev alive
//         epoll_event ev{};
//         ev.events = EPOLLIN | EPOLLONESHOT;
//         ev.data.fd = fd;
//
//         // after EPOLL_CTL_MOD success, epoll_wait may return this fd,
//         // so should be careful of the thread safety and memory visibility after EPOLL_CTL_MOD
//         if (epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
//             if (errno == ENOMEM || errno == ENOSPC) {
//                 warningWithErrno("FTP::callbackOnEndOfCmd epoll_ctl");
//                 errorHandler(fd);
//             } else {
//                 bugWithErrno(("FTP::callbackOnEndOfCmd, EPOLL_CTL_MOD, fd: " + to_string(fd)).c_str(), errno, true);
//             }
//         }
//     }
//
//     bool sendFailedMsg(int fd) {
//         const char msg[] = "500 Server Internal Error" END_OF_LINE;
//         return setNonBlocking(fd) && writeAllData(fd, msg, sizeof(msg) - 1);
//     }
//
//
//     // run on control thread
//     bool sendHelloMsg(int fd) sendHelloMsg(int fd) {
//         const char msg[] = "220 hzxFTP (0.01)" END_OF_LINE;
//         bool t = writeAllData(fd, msg, sizeof(msg) - 1);
//         // for that when sendHelloMsg, this fd havn't add to epoll, and have no session
//         // so should closeFileDescriptor directly;
//         if (!t) {
//             closeFileDescriptor(fd);
//         }
//         return t;
//     }
//
//     void openNewSession(int fd) {
//         auto *session = new Session(fd, callbackOnEndOfCmd, callbackOnEndOfSession, this->thisMachineIP);
//         if (!mutexLock(userRecordMutex)) {
//             bugWithErrno("FTP::openNewSession lock mutex failed", errno, true);
//         }
//         userRecord[fd] = session;
//         if (!mutexUnlock(userRecordMutex)) {
//             bugWithErrno("FTP::openNewSession unlock mutex failed", errno, true);
//         }
//     }
//
//     void gotoSessionHandler(int fd) {
//         // although just read the userRecord here,
//         // the lock is necessary to act as memory barrier.
//         if (!mutexLock(userRecordMutex)) {
//             bugWithErrno("FTP::gotoSessionHandler get userRecordMutex failed!", errno, true);
//         }
//         auto p = userRecord.find(fd);
//         if (p == userRecord.end()) {
//             bug(("FTP::gotoSessionHandler: an effective user cmd fd " + to_string(fd) +
//                  " has no correspond session").c_str(),
//                 true, __LINE__);
//         }
//         mutexUnlock(userRecordMutex);
//
//         if (!pool->addTask(task_t(userHandler, p->second))) {
//             errorHandler(fd);
//             warning("FTP gotoSessionHandler: one session be closed because of the full task queue");
//         }
//     }
//
//     void errorHandler(int fd) {
//         sendFailedMsg(fd);
//         destroySession(fd);
//     }
//
//
//     void destroySession(int fd) {
//         /*
//          * MUST make sure that, between the destroy process,
//          * the fd is not reuse by OS,
//          * so we MUST free the fd after all delete work.
//          *
//          * For that the user's cmd FD is EPOLLONESHOT,
//          * so we can make sure that when destroy the session,
//          * no other thread will invoke this session
//          */
//         if (!mutexLock(userRecordMutex)) {
//             bug("FTP::callbackOnEndOfSession: this userRecordMutex must not lock failed");
//         }
//         auto itp = userRecord.find(fd);
//         if (itp == userRecord.end()) {
//             bug(("FTP::closeFileDescriptor the fd's" + to_string(fd) +
//                  " correspond session is not exist").c_str());
//         } else {
//             userRecord.erase(itp);
//             delete itp->second;
//             if (!closeFileDescriptor(fd)) {
//                 bug(("FTP::delAndClearSession closeFileDescriptor fd:" + to_string(fd) +
//                      " should not failed").c_str());
//             }
//         }
//         if (epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr) < 0) {
//             bugWithErrno(("FTP::destroySession EPOLL_CTL_DEL failed, fd: " + to_string(fd)).c_str(), errno, true);
//         }
//         closeFileDescriptor(fd);
//         mutexUnlock(userRecordMutex);
//         atomic_fetch_sub<int>(currentFdCnt, 1);
//     }
//
//     /*
//      * This method run when SIGINT occur.
//      * So it can ONLY invoke async-signal-safe functions (read the CSAPP P767)
//      */
//     static void interruptHandler(int num) {
//         switch (num) {
//             case SIGINT: {
//                 willExit = true;
//                 const char msg[] = "Exiting...\n";
//                 write(STDERR_FILENO, msg, sizeof(msg) - 1);
//                 break;
//             }
//             case SIGPIPE: {
//                 const char msgPipe[] = "Broken Pipe\n";
//                 write(STDERR_FILENO, msgPipe, sizeof(msgPipe) - 1);
//                 break;
//             }
//             default: {
//                 const char msgUnexpect[] = "FTP::interruptHandler: call me with unexpected signal";
//                 write(STDERR_FILENO, msgUnexpect, sizeof(msgUnexpect) - 1);
//                 _exit(BUG_EXIT);
//             }
//         }
//     }
//
//     static sigset_t *makeSigSetForEpollWait(sigset_t *set) {
//         sigemptyset(set);
//         sigaddset(set, SIGINT);
//         return set;
//     }
//
//     /**
//      * @return whether, the errno is set appropriately
//      */
//     static bool addToEpollInterestList(int fd, int epollFd) {
//         epoll_event ev{};
//         ev.events = EPOLLIN;
//         ev.data.fd = fd;
//         return epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) < 0;
//     }
//
//
// private:
//     /*
//      * the userRecord is protected by userRecordMutex.
//      * when operate userRecord, MUST lock this mutex,
//      * no matter whether read or write.
//      * lock while read is to make the lock act as a memory barrier,
//      */
//     pthread_mutex_t userRecordMutex = PTHREAD_MUTEX_INITIALIZER;
//
//     /*
//      * map cmdFd to Session*
//      */
//     map<int, Session *> userRecord;
//
//     /**
//      * use currentFdCnt to control the online user number at the same time
//      * fd contain the listen fd and user's cmd fd
//      */
//     atomic<int> *currentFdCnt = 0;
//     ThreadPool *pool;
//     int epollFd = 0;
//
//     /**
//      * the 0 indicate the end of sigToBlock
//      */
//     const int sigToBlock[3] = {SIGPIPE, SIGINT, 0};
//     const string thisMachineIP = getThisMachineIp();
// private:
//     struct ArgvOfControlThread {
//         // store the this point
//         FTP *ftp;
//         int mainPort;
//
//         ArgvOfControlThread(FTP *ftp, int mainPort)
//                 : ftp(ftp), mainPort(mainPort) {}
//     };
// };
//
// #endif
