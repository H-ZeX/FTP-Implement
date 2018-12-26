/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    threadPool.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-23-15:12:44
*   Describe:
*
**********************************************************************************/

#include "threadPool.h"

ThreadPool::ThreadPool(size_t threadCnt, const int *sigToBlock) {
    if (maxThreadCnt() < threadCnt) {
        warning("the threadCnt param too large, use DEFAULT_THREAD_CNT (100)");
        threadCnt = DEFAULT_THREAD_CNT;
    }
    runningThreadCnt = 0;
    startedThreadCnt = 0;
    isShutdown = noShutdown;
    pthread_mutex_init(&taskMutex, nullptr);
    pthread_cond_init(&notify, nullptr);
    threadArray.reserve(threadCnt);
    argvArray.reserve(threadCnt);
    pthread_t k;
    for (int i = 0; i < threadCnt; i++) {
        argvArray.push_back(ThreadArgv(this, sigToBlock));
        if (createThread(k, &(ThreadPool::worker), &(*argvArray.rbegin()))) {
            threadArray.push_back(k);
            atomic_fetch_add<int>(&(this->startedThreadCnt), 1);
        }
    }
}

bool ThreadPool::addTask(const task_t &task) {
    if (task.isValid() == false) {
        bug("pass invalid task to ThreadPool::addTask", false);
        return false;
    }
    if (this->isShutdown.load() != noShutdown) {
        warning("ThreadPool::addTask: the shutdown func has been called, should not add more task");
        return false;
    }
    if (mutexLock(this->taskMutex) == false) {
        return false;
    }
    if (taskQue.size() >= MAX_TASK_CNT) {
        warning("ThreadPool::addTask taskQue is full");
        mutexUnlock(this->taskMutex);
        return false;
    } else {
        // if signal() is after unlock();
        // if just after push, a new thread try to test whether taskQue.size()==0
        // and get the task success
        // after that, the cond will signal another thread,
        // however, this thread will get no task
        taskQue.push(task);
        pthread_cond_signal(&this->notify);
        mutexUnlock(taskMutex);
        return true;
    }
}

void ThreadPool::shutdown(bool completeRest) {
    if (completeRest) {
        this->isShutdown = gracefulShutdown;
    } else {
        this->isShutdown = immediateShutdown;
    }
    // for that all thread may wait for new task, so i need to signal them
    pthread_cond_broadcast(&this->notify);
}

void *ThreadPool::worker(void *argv) {
    ThreadPool &tpool = *(((ThreadArgv *)argv)->pool);
    const int *p = ((ThreadArgv *)argv)->sigToBlock;
    if (p && changeThreadSigMask(p, SIG_BLOCK) == false) {
        warning("ThreadPool worker changeThreadSigMask failed, this thread exit");
        pthread_exit(nullptr);
    }
    while (true) {
        // get the mutex
        if (!mutexLock(tpool.taskMutex, MUTEX_TIMEOUT_SEC)) {
            atomic_fetch_sub<int>(&(tpool.startedThreadCnt), 1);
            return nullptr;
        }
        if (tpool.taskQue.size() == 0 && tpool.isShutdown.load() == noShutdown) {
            if (condWait(tpool.notify, tpool.taskMutex) == false) {
                mutexUnlock(tpool.taskMutex);
                atomic_fetch_sub<int>(&(tpool.startedThreadCnt), 1);
                return nullptr;
            }
        }
        // for that only the shutdown func call broadcast
        // so if taskQue is empty, and the condWait return true
        // one of these check will success
        if ((tpool.isShutdown.load() == immediateShutdown) ||
            (tpool.isShutdown.load() == gracefulShutdown && tpool.taskQue.size() == 0)) {
            mutexUnlock(tpool.taskMutex);
            atomic_fetch_sub<int>(&(tpool.startedThreadCnt), 1);
            return nullptr;
        }
        // when cond wait, some thread may wait on cond
        // some thread may just wait in the mutex at the beginning
        // if these thread run as long as the addTask() unlock mutex,
        // then the task that own by the thread who is wakeup from cond wait
        // will be stolen, so this taskQue may be 0 here;
        if (tpool.taskQue.size() == 0) {
            mutexUnlock(tpool.taskMutex);
            continue;
        }
        const task_t k = tpool.taskQue.front();
        tpool.taskQue.pop();
        mutexUnlock(tpool.taskMutex);
        atomic_fetch_add<int>(&(tpool.runningThreadCnt), 1);
        void *r = k.func(k.argument);
        if (k.callback) {
            k.callback(r);
        }
        atomic_fetch_sub<int>(&(tpool.runningThreadCnt), 1);
        if ((tpool.isShutdown.load() == immediateShutdown)) {
            atomic_fetch_sub<int>(&(tpool.startedThreadCnt), 1);
            return nullptr;
        }
    }
}

ThreadPool::~ThreadPool() {
    if (this->isShutdown.load() == noShutdown || this->runningThreadCnt.load() > 0 ||
        this->startedThreadCnt.load() > 0) {
        fprintf(stderr, "shutdown\n");
        this->shutdown();
    }
    // for that after sub startedThreadCnt, the thread may be interrupt,
    // so it haven't end
    for (int i = 0; i < threadArray.size(); i++) {
        joinThread(threadArray[i]);
    }
    condDestroy(this->notify);
    mutexDestroy(this->taskMutex);
}
