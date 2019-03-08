/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    threadUtility.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-23-15:36:09
*   Describe:
*
**********************************************************************************/
#include "ThreadUtility.h"

#include <iostream>
using namespace std;

bool mutexLock(pthread_mutex_t &mutex, int timeout) {
    int t;
    if (timeout <= 0) {
        WrapForFunc(pthread_mutex_lock(&mutex), 0, "mutexLock pthread_mutex_lock failed", false, t);
        return t == 0;
    } else {
        timespec timeoutTime{};
        WrapForFunc(clock_gettime(CLOCK_REALTIME, &timeoutTime), 0,
                    "mutexLock clock_gettime failed", true, t);
        if (t < 0) {
            return false;
        }
        timeoutTime.tv_sec += timeout;
        WrapForFunc(pthread_mutex_timedlock(&mutex, &timeoutTime), 0,
                    "mutexLock pthread_mutex_timedlock failed", false, t);
        return t == 0;
    }
}
bool mutexUnlock(pthread_mutex_t &mutex, bool isErrorChecking) {
    int t;
    if ((t = pthread_mutex_unlock(&mutex)) != 0) {
        bugWithErrno("mutexUnlock faile", t);
    }
    return t == 0;
}

bool mutexDestroy(pthread_mutex_t &mutex) {
    int t;
    WrapForFunc(pthread_mutex_destroy(&mutex), 0, "mutexDestroy pthread_mutex_destroy failed",
                false, t);
    return t == 0;
}

bool condWait(pthread_cond_t &cond, pthread_mutex_t &mutex, int timeout) {
    int t;
    if (timeout <= 0) {
        // pthread_cond_wait never return an error code
        pthread_cond_wait(&cond, &mutex);
        return true;
    } else {
        timespec timeoutTime{};
        WrapForFunc(clock_gettime(CLOCK_REALTIME, &timeoutTime), 0,
                    "mutexLock clock_gettime failed", true, t);
        if (t < 0) {
            return false;
        }
        timeoutTime.tv_sec += timeout;
        errno_t ea[] = {EINTR, 0};
        timeoutTime.tv_sec += 2;
        errnoRetryV2(pthread_cond_timedwait(&cond, &mutex, &timeoutTime), ea,
                      "condWait pthread_cond_timedwait failed", t);
        if (t != 0 && t != EINTR) {
            warningWithErrno("condWait pthread_cond_timedwait failed", t);
        }
        return t == 0;
    }
}
bool condDestroy(pthread_cond_t &cond) {
    int t;
    if ((t = pthread_cond_destroy(&cond)) != 0) {
        warningWithErrno("condDestroy pthread_cond_destroy failed", t);
    }
    return t == 0;
}

bool createThread(pthread_t &pid, void *(*run)(void *), void *argv, const pthread_attr_t *attr) {
    int t;
    WrapForFunc(pthread_create(&pid, attr, run, argv), 0, "pthread_create failed", false, t);
    return t == 0;
}

bool joinThread(pthread_t pid, void **retval) {
    int t = pthread_join(pid, retval);
    if (t < 0) {
        warningWithErrno("joinThread pthread_join failed");
    }
    return t == 0;
}

bool rwlockRdlock(pthread_rwlock_t &lock, int time) {
    int t;
    if (time <= 0) {
        if ((t = pthread_rwlock_rdlock(&lock)) < 0) {
            warningWithErrno("rwlockRdlock pthread_rwlock_rdlock failed", t);
        }
        return t == 0;
    } else {
        timespec timeoutTime{};
        WrapForFunc(clock_gettime(CLOCK_REALTIME, &timeoutTime), 0,
                    "mutexLock clock_gettime failed", true, t);
        if (t < 0) {
            return false;
        }
        timeoutTime.tv_sec += time;
        if ((t = pthread_rwlock_timedrdlock(&lock, &timeoutTime)) < 0) {
            warningWithErrno("rwlockRdlock pthread_rwlock_timedrdlock faild", t);
        }
        return t == 0;
    }
}

bool rwlockWrlock(pthread_rwlock_t &lock, int time) {
    int t;
    if (time <= 0) {
        if ((t = pthread_rwlock_wrlock(&lock)) < 0) {
            warningWithErrno("rwlockWrlock pthread_rwlock_wrlock failed", t);
        }
        return t == 0;
    } else {
        timespec timeoutTime{};
        WrapForFunc(clock_gettime(CLOCK_REALTIME, &timeoutTime), 0,
                    "mutexLock clock_gettime failed", true, t);
        if (t < 0) {
            return false;
        }
        timeoutTime.tv_sec += time;
        if ((t = pthread_rwlock_timedwrlock(&lock, &timeoutTime)) < 0) {
            warningWithErrno("rwlockWrlock pthread_rwlock_timedwrlock faild", t);
        }
        return t == 0;
    }
}
bool rwlockUnlock(pthread_rwlock_t &lock) {
    int t;
    if ((t = pthread_rwlock_unlock(&lock)) < 0) {
        bugWithErrno("rwlockUnlock pthread_rwlock_unlock failed", t);
    }
    return true;
}

bool changeThreadSigMask(const int sigSet[], int how) {
    sigset_t set;
    sigemptyset(&set);
    for (int i = 0; sigSet[i] && i < 30; i++) {
        if (sigaddset(&set, sigSet[i]) < 0) {
            warningWithErrno("changeThreadSigMask sigaddset");
            return false;
        }
    }
    int s = pthread_sigmask(how, &set, nullptr);
    if (s != 0) {
        warningWithErrno("changeThreadSigMask pthread_sigmask", s);
        return false;
    }
    return true;
}
