/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    ftp.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-16-16:54:07
*   Describe:
*
**********************************************************************************/
#include "ftp.h"

pthread_mutex_t FTP::userRecordMutex = PTHREAD_MUTEX_INITIALIZER;
map<int, Session *> FTP::userRecord = map<int, Session *>();
ThreadPool *FTP::pool = new ThreadPool(threadCnt, sigToBlock);
atomic<int> *FTP::currentFdCnt = new atomic<int>(0);
bool FTP::willExit = false;
int FTP::epollFd = -1;
const int FTP::sigToBlock[] = {SIGPIPE, SIGINT, 0};
const string FTP::thisMachieIP = getThisMachineIp();

void FTP::start(int port) {
    string p = std::to_string(port);
    pthread_t pid;
    if (!changeThreadSigMask(sigToBlock, SIG_BLOCK)) {
        return;
    } else if (!createThread(pid, listenOnMainPort, &p)) {
        return;
    } else {
        signal(SIGINT, interruptHandler);
        fprintf(stderr, "hzxFTP server run on %d\nuse Ctrl-C to close the server\n", port);
        if (!joinThread(pid)) {
            return;
        }
    }
}
// if don't use { } on case, compiler error
void FTP::interruptHandler(int num) {
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
void *FTP::listenOnMainPort(void *argv) {
    const string &mainPort = *(string *)argv;
    epoll_event ev{}, evArray[MAX_EPOLL_SIZE];
    int mainFd;
    if ((mainFd = openListenfd(mainPort.c_str())) < 0) {
        warning(("FTP::listenOnMainPort open " + mainPort + " port failed").c_str());
        exit(OPEN_MAINFD_ERROR);
    }
    fprintf(stderr, "open mainfd %d\n", mainFd);
    if ((epollFd = epoll_create(MAX_EPOLL_SIZE)) < 0) {
        warningWithErrno("FTP::listenOnMainPort epoll_create");
        exit(EPOLL_CREATE_ERROR);
    }
    // if mainfd is blocking, the ftp server will still work, so I don't call fatalError here
    // actually, I don't very understand that the effect of nonblocking for mainFd
    // i just do this according to the epoll's manual
    // if use epollet, should make the fd nonblocking
    setNonBlocking(mainFd);
    fprintf(stderr, "FTP::listenOnMainPort is mainfd blocking: %d\n", isBlocking(mainFd));
    // if use EPOLLET here, when many connect come fastly,
    // mainFd will not return from epoll_wait
    ev.events = EPOLLIN; // | EPOLLET;
    ev.data.fd = mainFd;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, mainFd, &ev) < 0) {
        warningWithErrno("FTP::listenOnMainPort epoll_ctl");
        exit(EPOLL_ADD_MAINFD_ERROR);
    }
    atomic_fetch_add<int>(currentFdCnt, 1);
    int waitFdCnt = 0;
    while (true) {
        if ((waitFdCnt = epoll_pwait(epollFd, evArray, currentFdCnt->load(), -1,
                                     makeSigSetForEpollWait())) < 0 &&
            errno != EINTR) {
            warningWithErrno("FTP::listenOnMainPort epoll_wait");
            exit(EPOLL_WAIT_ERROR);
        } else if (errno == EINTR && !willExit) {
            fprintf(stderr, "epoll_wait EINTR\n");
            continue;
        } else if (willExit) {
            fprintf(stderr, "epoll_wait EINTR\n");
            break;
        }

        flockfile(stderr);
        fprintf(stderr, "epoll_wait return: ");
        for (int i = 0; i < waitFdCnt; i++) {
            fprintf(stderr, "isblocking %d: %d\n", evArray[i].data.fd,
                    isBlocking(evArray[i].data.fd));
        }
        funlockfile(stderr);

        for (int i = 0; i < waitFdCnt; i++) {
            if (evArray[i].data.fd != mainFd) {
                gotoSessionHandler(evArray[i].data.fd);
            } else {
                openNewSession(mainFd);
            }
        }
    }
    return nullptr;
}
// run on mutiple thread
void *FTP::userHandler(void *argv) {
    Session &s = *(Session *)argv;
    s.handle();
    return nullptr;
}
// run on multiple thread
// @note that del the fd from epollFd should before the clear of userRecord
// (actually, can clear before del(but they should run togester in this func),
// for that, i use EPOLLONESHOT on this fd, however,  del before clear will better);
void FTP::callbackOnEndOfSession(void *argv) {
    fprintf(stderr, "Come to FTP::callbackOnEndOfSession\n");
    int fd = ((Session *)argv)->getCmdFd();
    if (fd < 3) {
        bug(("FTP::callbackOnEndOfSession " + to_string(fd)).c_str());
    }

    // in what case will EPOLL_CTL_DEL failed, in such cases, would the fd
    // still in the epoll wait set
    int t = epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    if (t < 0) {
        warningWithErrno("FTP::callbackOnEndOfSession epoll_ctl");
        // bug("Can not epoll del faild");
    } else {
        delAndClearSession(fd); // tag
    }
}
// run on mutiple thrad
void FTP::callbackOnEndOfCmd(void *argv) {
    int fd = ((Session *)argv)->getCmdFd();
    fprintf(stderr, "FTP::callbackOnEndOfCmd: fd: %d, cmd: %s\n", fd, ((Session *)argv)->cmdBuf);
    if (fd < 3) {
        bug(("FTP::callbackOnEndOfCmd " + to_string(fd)).c_str());
    }
    // if the fd is blocking, the ftp server can still work;
    setNonBlocking(fd);
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.fd = fd;
    int t = epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
    if (t < 0) {
        warningWithErrno("FTP::callbackOnEndOfCmd epoll_ctl");
        errorHandler(fd);
    }
    return;
}
// run on control thread
bool FTP::sendHelloMsg(int fd) {
    const char msg[] = "220 hzxFTP (0.01)" END_OF_LINE;
    bool t = writeAllData(fd, msg, sizeof(msg) - 1);
    // for that when sendHelloMsg, this fd havn't add to epoll, and have no session
    // so should closeFileDescriptor directly;
    if (!t) {
        closeFileDescriptor(fd);
    }
    return t;
}
bool FTP::sendFailedMsg(int fd) {
    const char msg[] = "500 Server Internal Error" END_OF_LINE;
    return setNonBlocking(fd) && writeAllData(fd, msg, sizeof(msg) - 1);
}
// run on control thread
//
// accept->new session->add to userRecord->add to epoll wait
void FTP::openNewSession(int mainFd) {
    int newFd;
    if (currentFdCnt->load() >= MAX_EPOLL_SIZE || (newFd = acceptConnect(mainFd)) < 0 ||
        !sendHelloMsg(newFd)) {
        fprintf(stderr, "Open New Session failed %d %d\n", currentFdCnt->load(), MAX_EPOLL_SIZE);
        return;
    }
    Session *tmp = new (std::nothrow) Session(newFd, callbackOnEndOfCmd, callbackOnEndOfSession, thisMachieIP);
    if (tmp == nullptr) {
        warning("FTP::openNewSession new session failed");
        sendFailedMsg(newFd);
        closeFileDescriptor(newFd);
        return;
    }
    mutexLock(userRecordMutex);
    if (userRecord[newFd] != nullptr) {
        bug(("FTP::listenOnMainPort: when add to userRecord, this new fd" + to_string(newFd) +
             " had had its session")
                .c_str());
    }
    userRecord[newFd] = tmp;
    mutexUnlock(userRecordMutex);
    // if this fd is blocking, the ftp server will still work,
    // so I don't call fatalError here
    setNonBlocking(newFd);
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.fd = newFd;
    int t = epoll_ctl(epollFd, EPOLL_CTL_ADD, newFd, &ev);
    if (t < 0) {
        warningWithErrno("FTP::openNewSession epoll_ctl");
        sendFailedMsg(newFd);
        delAndClearSession(newFd);
        return;
    } else {
        atomic_fetch_add<int>(currentFdCnt, 1);
        fprintf(stderr, "\e[31mnew session fd: %d, session: %p\n\e[0m", newFd, tmp);
        fprintf(stdout, "new session fd: %d, session: %p\n", newFd, tmp);
        fflush(stdout);
    }
}
// run on control thread
void FTP::gotoSessionHandler(int fd) {
    mutexLock(userRecordMutex);
    auto p = userRecord.find(fd);
    if (p == userRecord.end()) {
        bug(("FTP::gotoSessionHandler: an avaliable user cmd fd" + to_string(fd) +
             " have no correspond session")
                .c_str(),
            true, __LINE__);
    }
    mutexUnlock(userRecordMutex);

    fprintf(stderr, "threadPool add_task\n");
    if (!setBlocking(fd)) {
        errorHandler(fd);
        return;
    }
    if (!pool->addTask(task_t(userHandler, p->second))) {
        errorHandler(fd);
        warning("FTP gotoSessionHandler: one session be closed because of the full task queue");
    }
}
void FTP::errorHandler(int fd) {
    sendFailedMsg(fd);
    // if delete the fd from epollFd failed,
    // I will not delete the session, or there will be fatalError;
    int t = epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    if (t < 0) {
        warningWithErrno("FTP::errorHandler epoll_ctl");
        return;
    } else {
        delAndClearSession(fd);
    }
}
void FTP::delAndClearSession(int fd) {
    if (!mutexLock(userRecordMutex)) {
        bug("FTP::callbackOnEndOfSession: this userRecordMutex must not lock failed");
    }
    auto itp = userRecord.find(fd);
    void *ptr = itp->second;
    if (itp == userRecord.end()) {
        bug(("FTP::closeFileDescriptor the fd's" + to_string(fd) +
             " correspond session is not exist")
                .c_str());
    } else {
        // erase must before the delete, for that,
        // if this func doesn't run on control thread,
        // and that after delete, switch to control thread,
        // for that had been deleted, the fd is free, so can be reused
        // however, userRecord[fd] is not nullptr, this will lead to call the bug()
        // if delete be interrupt, then the check whether userRecord[fd]==nullptr will pass
        // however, after switch to continue to run delete, the new session will be free
        userRecord.erase(itp);
        delete itp->second;
        if (!closeFileDescriptor(fd)) {
            bug(("FTP::delAndClearSession closeFileDescriptor fd:" + to_string(fd) +
                 " should not failed")
                    .c_str());
        }
    }
    mutexUnlock(userRecordMutex);
    atomic_fetch_sub<int>(currentFdCnt, 1);

    mutexLock(userRecordMutex);
    fprintf(stderr, "\e[31mFTP::delAndClearSession fd: %d, session: %p,"
                    " itp: %p userRecordMutex: %d\n\e[0m",
            fd, ptr, itp->second, userRecord.find(fd) == userRecord.end());
    fprintf(stdout, "FTP::delAndClearSession fd: %d, session: %p,"
                    " itp: %p userRecordMutex: %d\n",
            fd, ptr, itp->second, userRecord.find(fd) == userRecord.end());
    fflush(stdout);
    mutexUnlock(userRecordMutex);
}
sigset_t *FTP::makeSigSetForEpollWait() {
    static sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    return &set;
}
