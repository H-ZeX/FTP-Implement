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

#include "def.h"
#include "utility.h"
#include <pthread.h>
#include <time.h>

/**
 * @return whether success
 */
bool mutexLock(pthread_mutex_t &mutex, int timeout = 0);
/**
 * TODO I haven't understand errorChecking type mutex,
 * so its errno's handler should be rewrite
 */
bool mutexUnlock(pthread_mutex_t &mutex, bool isErrorChecking = false);
bool mutexDestroy(pthread_mutex_t &mutex);
bool condWait(pthread_cond_t &cond, pthread_mutex_t &mutex, int timeout = 0);
bool condDestroy(pthread_cond_t &cond);
bool createThread(pthread_t &pid, void *(*run)(void *),  void *argv = nullptr,
                  const pthread_attr_t *attr = nullptr);
bool joinThread(pthread_t pid, void **retval = nullptr);
bool rwlockRdlock(pthread_rwlock_t &lock, int time = -1);
bool rwlockWrlock(pthread_rwlock_t &lock, int time = -1);
/**
 * If  an  implementation  detects that the value specified by the rwlock
 * argument to pthread_rwlock_unlock() does not refer to  an  initialized
 * read-write  lock  object,  it  is recommended that the function should
 * fail and report an [EINVAL] error.
 *
 * If an implementation detects that the value specified  by  the  rwlock
 * argument to pthread_rwlock_unlock() refers to a read-write lock object
 * for which the current thread does not hold a lock, it  is  recommended
 * that the function should fail and report an [EPERM] error.
 *
 * so if there is error occure, this func will call bugWithErrno
 * and it will exit the process
 */
bool rwlockUnlock(pthread_rwlock_t &lock);

/**
 * sigSet is end with 0 to indicate the end of sigSet
 * for that all signal is >0
 * if no 0 if found, the max len of sigSet is 30
 *
 * MT-safe
 */
bool changeThreadSigMask(const int sigSet[], int how);
#endif
