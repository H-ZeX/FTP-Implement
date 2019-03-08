/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    ftp.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-16-16:54:07
*   Describe:
*
**********************************************************************************/
#include "FTP.h"

void FTP::start(int port) {
    string p = std::to_string(port);
    pool->start();
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

}
void *FTP::listenOnMainPort(void *argv) {
}

void FTP::callbackOnEndOfSession(void *argv) {
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
    Session *tmp = new (std::nothrow) Session(newFd, callbackOnEndOfCmd, callbackOnEndOfSession, thisMachineIP);
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
    epoll_event ev{};
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
}
