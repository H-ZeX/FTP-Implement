/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    ftp.h
*   Author:       H-ZeX
*   Create Time:  2018-08-14-11:01:24
*   Describe:
*
**********************************************************************************/

#ifndef __FTP_H__
#define __FTP_H__

#include "src/util/def.h"
#include "src/util/netUtility.h"
#include "session.h"
#include "src/tools/threadPool.h"
#include "src/util/utility.h"
#include <csignal>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * NOT MT-safe !
 *
 * **should only call start func one time
 * or its behaviour is undefined**
 *
 * @note some data says that epoll_ctl is MT-safe even if use same epollFd
 * I believe this and use epoll_ctl in multi thread
 * for that the session's cmdFd should use EPOLLONESHOT,
 * so when return from session handle (session handle only handle one cmd one time)
 * I need to explicit call epoll_ctl to restart this session cmdFd
 * this means that, if use lock in epoll_ctl, the performance will to low
 * in this case, every session own one thread will better than threadPool
 *
 * @note be careful that after a fd (not mainfd) has its session,
 * it should only be closed at the destructors of session object
 * (when it has no session, (in the openNewSession func), should call closeFileDescriptor);
 * this is important, for that if close the cmdFd before delete
 * the session object or before delete the fd from epollFd,
 * then this new connect may use have a fd whose value is same as this fd
 * this means that this new connect will use a old session,
 * and the session may be deleted when this new connect still alive.
 *
 * @note all thread is block SIGPIPE and SIGINT, except that,
 * when use epoll_wait only SIGPIPE is block
 * for that, the signal may not send to control thread
 * so even if use signal to register the handler, the epoll_wait in control thread
 * may no recv the signal and still wait;
 * so i need to forbid other thread to recv signal except epoll_wait is run
 */
class FTP {
  public:
    ~FTP() {
        for (auto &it : userRecord) {
            if (it.second != nullptr) {
                delete (it.second);
            }
        }
        delete FTP::pool;
        delete FTP::currentFdCnt;
    }
    /**
     * for each process, this func can only be called one time
     * otherwise, the behavior is undefined;
     */
    void start(int port = 21);

  private:
    static void *listenOnMainPort(void *argv);
    static void *userHandler(void *argv);
    static void callbackOnEndOfSession(void *argv);
    /**
     * should only be call with valid param
     * for that can't code to check whether it is valid;
     */
    static void callbackOnEndOfCmd(void *argv);
    static bool sendFailedMsg(int fd);
    static bool sendHelloMsg(int fd);
    static void openNewSession(int mainFd);
    static void gotoSessionHandler(int fd);
    static sigset_t *makeSigSetForEpollWait();
    /**
     * MT-safe
     * however, for that this func call delAndClearSession,
     * so should not lock the userRecordMutex;
     *
     * can only use when the fd has its session and in the epoll wait set
     *
     * will send the error message to client and close session
     * for that this func may be called in control thread
     * so i make the fd nonblocking before send the error message
     */
    static void errorHandler(int fd);
    /**
     * MT-safe
     * however, for that this func use userRecordMutex
     * so should not lock this mutex before call delAndClearSession
     */
    static void delAndClearSession(int fd);
    static void interruptHandler(int num);

  private:
    static pthread_mutex_t userRecordMutex;
    static map<int, Session *> userRecord; // map cmdFd to Session*
    static bool willExit;
    /**
     * currentFdCnt only be manager by delAndClearSession and openNewSession
     * (and, when open mainfd, will add one to it);
     */
    static atomic<int> *currentFdCnt;
    static ThreadPool *pool;
    static int epollFd;

    static const int threadCnt = 5;
    /**
     * the 0 indicate the end of sigToBlock
     */
    static const int sigToBlock[];
    static const string thisMachineIP;
};
#endif
