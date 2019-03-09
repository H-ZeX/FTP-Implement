/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    threadUtility.h
*   Author:       H-ZeX
*   Create Time:  2018-08-23-15:35:59
*   Describe:
*
**********************************************************************************/

#ifndef __THREAD_UTILITY_H__
#define __THREAD_UTILITY_H__

#include "Def.hpp"
#include "Utility.hpp"
#include <pthread.h>
#include <time.h>

bool mutexInit(pthread_mutex_t &mutex, const pthread_mutexattr_t *attr = nullptr) {
    int t = pthread_mutex_init(&mutex, attr);
    if (t != 0 && (t == EPERM || t == EINVAL)) {
        bugWithErrno("mutexInit pthread_mutex_init failed", t, true);
    } else if (t != 0) {
        warningWithErrno("mutexInit pthread_mutex_init", t);
    }
    return t == 0;
}

/**
 * @param timeout unit is second.
 * @return If timeout is >=0, then the return whether lock success.
 */
bool mutexLock(pthread_mutex_t &mutex, int timeout = -1) {
    if (timeout < 0) {
        int t = pthread_mutex_lock(&mutex);
        if (t != 0) {
            bugWithErrno("mutexLock pthread_mutex_lock failed", t, true);
        }
        return true;
    } else {
        timespec timeoutTime{};
        int t = clock_gettime(CLOCK_REALTIME, &timeoutTime);
        if (t < 0) {
            bugWithErrno("mutexLock clock_gettime failed", errno, true);
        }
        timeoutTime.tv_sec += timeout;
        t = pthread_mutex_timedlock(&mutex, &timeoutTime);
        if (t != 0) {
            if (t == ETIMEDOUT) {
                return false;
            } else {
                bugWithErrno("mutexLock pthread_mutex_timedlock failed", t, true);
            }
        }
        return true;
    }
}

void mutexUnlock(pthread_mutex_t &mutex) {
    int t = pthread_mutex_unlock(&mutex);
    if (t != 0) {
        bugWithErrno("mutexUnlock failed", t, true);
    }
}

void mutexDestroy(pthread_mutex_t &mutex) {
    int t = pthread_mutex_destroy(&mutex);
    if (t != 0) {
        bugWithErrno("mutexDestroy pthread_mutex_destroy failed", t, true);
    }
}

bool conditionInit(pthread_cond_t &condition, const pthread_condattr_t *attr = nullptr) {
    int t = pthread_cond_init(&condition, attr);
    if (t != 0 && t != ENOMEM) {
        bugWithErrno("conditionInit pthread_cond_init failed", t, true);
    } else if (t != 0) {
        warningWithErrno("conditionInit pthread_cond_init failed", t);
    }
    return t == 0;
}


/**
 * @return if timeout>=0, return whether wait success.
 */
bool conditionWait(pthread_cond_t &cond, pthread_mutex_t &mutex, int timeout = -1) {
    if (timeout < 0) {
        int t = pthread_cond_wait(&cond, &mutex);
        if (t != 0) {
            bugWithErrno("conditionWait pthread_cond_wait failed", t, true);
        }
        return true;
    } else {
        timespec timeoutTime{};
        int t = clock_gettime(CLOCK_REALTIME, &timeoutTime);
        if (t < 0) {
            bugWithErrno("conditionWait clock_gettime failed", errno, true);
        }
        timeoutTime.tv_sec += timeout;
        timeoutTime.tv_sec += 2;
        t = pthread_cond_timedwait(&cond, &mutex, &timeoutTime);
        if (t != 0 && t != ETIMEDOUT) {
            bugWithErrno("conditionWait pthread_cond_timedwait failed", t, true);
        }
        return t == 0;
    }
}

void conditionDestroy(pthread_cond_t &cond) {
    int t = pthread_cond_destroy(&cond);
    if (t != 0) {
        bugWithErrno("conditionDestroy pthread_cond_destroy failed", t, true);
    }
}

void conditionSignal(pthread_cond_t &cond) {
    // TODO the behavior of signal is different from manual
    // according to manual https://linux.die.net/man/3/pthread_cond_signal
    // if call pthread_cond_signal with an uninitialized condition.
    // it will return an EINVAL. however, in my test, it block.
    // the original text
    // The pthread_cond_broadcast() and pthread_cond_signal() function may fail if:
    // EINVAL
    // The value cond does not refer to an initialized condition variable.

    int t = pthread_cond_signal(&cond);
    if (t != 0) {
        bugWithErrno("conditionSignal "
                     "pthread_cond_signal failed",
                     t, true);
    }
}

void conditionBroadcast(pthread_cond_t &cond) {
    // TODO the behavior of broadcast is different manual
    // see the conditionSignal comment
    int t = pthread_cond_broadcast(&cond);
    if (t != 0) {
        bugWithErrno("conditionBroadcast "
                     "pthread_cond_broadcast failed",
                     t, true);
    }
}

/**
 * @return whether create success.
 */
bool createThread(pthread_t &pid,
                  void *(*run)(void *),
                  void *argv = nullptr,
                  const pthread_attr_t *attr = nullptr) {

    int t = pthread_create(&pid, attr, run, argv);
    if (t == EINVAL || t == EPERM) {
        bugWithErrno("createThread pthread_create failed", t, true);
    }
    return t == 0;
}

void joinThread(pthread_t pid, void **retVal = nullptr) {
    int t = pthread_join(pid, retVal);
    if (t < 0) {
        bugWithErrno("joinThread pthread_join failed", t, true);
    }
}

/**
 * @param sigSet end with 0 to indicate the end of sigSet, for that all signal is >0.
 * The max len of sigSet is 30.
 * @param how
 * <br/>
 * The behavior of the call is dependent on the value of how, as follows.
 * <br/>
 * SIG_BLOCK
 * <br/>
 * The set of blocked signals is the union of the current set and the set argument.
 * <br/>
 * SIG_UNBLOCK
 * <br/>
 * The signals in set are removed from the current set of blocked signals.
 * It is permissible to attempt to unblock a signal which is not blocked.
 * <br/>
 * SIG_SETMASK
 * <br/>
 * The set of blocked signals is set to the argument set.
 * <br/>
 *
 * MT-safe
 */
void changeThreadSigMask(const int sigSet[], int how) {
    sigset_t set;
    sigemptyset(&set);
    for (int i = 0; sigSet[i] && i < 30; i++) {
        if (sigaddset(&set, sigSet[i]) < 0) {
            bugWithErrno("changeThreadSigMask sigaddset", errno, true);
        }
    }
    int s = pthread_sigmask(how, &set, nullptr);
    if (s != 0) {
        bugWithErrno("changeThreadSigMask pthread_sigmask", s, true);
    }
}

#endif
