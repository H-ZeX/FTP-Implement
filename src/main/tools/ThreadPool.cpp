/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    threadPool.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-23-15:12:44
*   Describe:
*
**********************************************************************************/

#include "ThreadPool.h"

/**
 * this threadPool is fix threadPool
 * @param threadCnt the thread cnt of inside this pool,
 * if it >= the
 * @param sigToBlock all thread inside this pool will block these signals.
 * note that this array should end with 0
 */
ThreadPool::ThreadPool(size_t threadCnt, const int *sigToBlock)
        : threadCnt(computeThreadCnt(threadCnt)),
          sigToBlock(sigToBlock) {
    runningThreadCnt = 0;
    startedThreadCnt = 0;
    isShutdown = RUNNING;
    pthread_mutex_init(&taskMutex, nullptr);
    pthread_mutex_init(&threadArrayMutex, nullptr);
    pthread_cond_init(&notify, nullptr);
    threadArray.reserve(threadCnt);
    argvArray.reserve(threadCnt);
}

void ThreadPool::start() {
    /*
     * this method is to createThread.
     * according to Java concurrency in practice,
     * should NOT start thread in constructor, or the `this` point will escape.
     * which may make another thread see the incomplete construct object.
     */

    for (size_t i = 0; i < threadCnt; i++) {
        argvArray.emplace_back(this, sigToBlock);
    }
    pthread_t k;
    for (size_t i = 0; i < threadCnt; i++) {
        if (createThread(k, &(ThreadPool::worker), &(argvArray[i]))) {
            threadArray.push_back(k);
            atomic_fetch_add<int>(&(this->startedThreadCnt), 1);
        }
    }
}

bool ThreadPool::addTask(const task_t &task) {
    // if the caller pass an incomplete constructed task object to this method,
    // then this method may fail, but this method can not avoid this case,
    // this is not this method's responsibility
    if (!task.isValid()) {
        bug("pass invalid task to ThreadPool::addTask", false);
        return false;
    }
    if (this->isShutdown.load() != RUNNING) {
        warning("ThreadPool::addTask: the shutdown func has been called, should not add more task");
        return false;
    }
    if (!mutexLock(this->taskMutex)) {
        return false;
    }
    if (taskQue.size() >= MAX_TASK_CNT) {
        warning("ThreadPool::addTask taskQue is full");
        mutexUnlock(this->taskMutex);
        return false;
    } else {
        // the unlock call must after signal call
        taskQue.push(task);
        pthread_cond_signal(&this->notify);
        mutexUnlock(taskMutex);
        return true;
    }
}

void ThreadPool::shutdown(bool completeRest) {
    if (completeRest) {
        this->isShutdown = GRACEFUL_SHUTDOWN;
    } else {
        this->isShutdown = IMMEDIATE_SHUTDOWN;
    }
    // for that all thread may wait for new task, so i need to signal them
    pthread_cond_broadcast(&this->notify);
}

// TODO should add a new thread when a thread exit
void *ThreadPool::worker(void *argv) {
    /*
     * if the shutdown is called, then the thread wait at cond will response immediately,
     * the thread that run task will run until the task end and response immediately
     */

    ThreadArgv &threadArgv = *(ThreadArgv *) argv;
    ThreadPool &pool = *(threadArgv.pool);
    const int *p = threadArgv.sigToBlock;

    if (p && !changeThreadSigMask(p, SIG_BLOCK)) {
        // if can not change the signal, return
        warning("ThreadPool worker changeThreadSigMask failed, this thread exit");
        atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
        return nullptr;
    }
    while (true) {
        // use timeout to avoid dead lock
        if (!mutexLock(pool.taskMutex, MUTEX_TIMEOUT_SEC)) {
            warning("there may be an dead lock on this taskMutex, ThreadPool::worker");
            // if get lock failed, exit this thread
            atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
            return nullptr;
        }
        // when return from cond wait, should check whether the cond is still true
        while (pool.taskQue.empty() && pool.isShutdown.load() == RUNNING) {
            if (!condWait(pool.notify, pool.taskMutex)) {
                // if cond wait fail, exit this thread
                mutexUnlock(pool.taskMutex);
                atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
                return nullptr;
            }
        }
        // for that only the shutdown func call broadcast,
        // so if taskQue is empty, and the condWait return true,
        // one of these check will success
        if ((pool.isShutdown.load() == IMMEDIATE_SHUTDOWN) ||
            (pool.isShutdown.load() == GRACEFUL_SHUTDOWN && pool.taskQue.empty())) {
            mutexUnlock(pool.taskMutex);
            atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
            return nullptr;
        }
        const task_t k = pool.taskQue.front();
        pool.taskQue.pop();
        mutexUnlock(pool.taskMutex);
        atomic_fetch_add<int>(&(pool.runningThreadCnt), 1);
        void *r = k.func(k.argument);
        if (k.callback) {
            k.callback(r);
        }
        atomic_fetch_sub<int>(&(pool.runningThreadCnt), 1);
        if ((pool.isShutdown.load() == IMMEDIATE_SHUTDOWN)) {
            atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
            return nullptr;
        }
    }
}

ThreadPool::~ThreadPool() {
    if (this->isShutdown.load() == RUNNING || this->runningThreadCnt.load() > 0 ||
        this->startedThreadCnt.load() > 0) {
        fprintf(stderr, "shutdown\n");
        this->shutdown();
    }

    // should lock this mutex, this mutex is like memory barrier
    mutexLock(threadArrayMutex);
    for (unsigned long i : threadArray) {
        joinThread(i);
    }
    mutexUnlock(threadArrayMutex);

    condDestroy(this->notify);
    mutexDestroy(this->taskMutex);
    mutexDestroy(this->threadArrayMutex);
}

size_t ThreadPool::computeThreadCnt(size_t threadCnt) {
    if (maxThreadCnt() < threadCnt) {
        return DEFAULT_THREAD_CNT;
    }
    return threadCnt;
}
