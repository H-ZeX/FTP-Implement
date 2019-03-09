/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    threadPool.h
*   Author:       H-ZeX
*   Create Time:  2018-08-23-15:12:49
*   Describe:
*
**********************************************************************************/

#include "src/main/util/Def.hpp"
#include "src/main/util/ThreadUtility.hpp"
#include "src/main/util/utility.hpp"
#include <atomic>
#include <pthread.h>
#include <queue>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <cassert>

using namespace std;

/*
 * the pthread can only accept static function,
 * so this lock must be global(static global) variable.
 */
static pthread_mutex_t argvArrayMutex;

/*
 * This is used by signal handler an threadPool,
 * so it must be global
 */
static volatile bool willExit = false;

/**
 * This threadPool is a fix threadPool.
 * <br/>
 * This class is thread safe.
 * This class can only Initialize one time, So I use Singleton mode.
 */
class ThreadPool {
public:
    /**
     * @param sigToBlock Should end with 0, or there may be buffer overload.
     * The max len of this array is 30.
     * <br/>
     * The thread inside pool will only block signal on sigToBlock.
     */
    explicit ThreadPool(size_t threadCnt = DEFAULT_THREAD_CNT,
                        const int *sigToBlock = nullptr)
            : threadCnt(computeThreadCnt(threadCnt)),
              sigToBlock(sigToBlock) {
        // make sure that the `this` point is NOT escaped
        runningThreadCnt = 0;
        startedThreadCnt = 0;
        isShutdown = RUNNING;
        if (!mutexInit(argvArrayMutex)
            || !mutexInit(taskMutex)
            || !mutexInit(threadArrayMutex)
            || !conditionInit(notify)) {
            warning("init threadPool failed");
            return;
        }
        threadArray.reserve(threadCnt);
        argvArray.reserve(threadCnt);
    }

    /**
     * start this threadPool
     */
    void start() {
        /*
         * this method is to createThread.
         * according to Java concurrency in practice,
         * should NOT start thread in constructor, or the `this` point will escape.
         * which may make another thread see the incomplete construct object.
         */

        // act as memory barrier
        mutexLock(argvArrayMutex);
        for (size_t i = 0; i < threadCnt; i++) {
            argvArray.emplace_back(this, sigToBlock);
        }
        mutexUnlock(argvArrayMutex);

        // act as memory barrier
        mutexLock(threadArrayMutex);
        for (size_t i = 0; i < threadCnt; i++) {
            pthread_t tid;
            if (createThread(tid, &(ThreadPool::worker), &(argvArray[i]))) {
                threadArray.push_back(tid);
                atomic_fetch_add<int>(&(this->startedThreadCnt), 1);
            }
        }
        mutexUnlock(threadArrayMutex);
    }

    /**
     * @return whether addTask success,
     * will fail when the taskQue is full or had called shutdown.
     * this method is thread safe
     */
    bool addTask(const task_t &task) {
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

    /**
     * @param completeRest whether complete the rest of task.
     * this method is thread safe
     */
    void shutdown(bool completeRest = true) {
        if (completeRest) {
            this->isShutdown = GRACEFUL_SHUTDOWN;
        } else {
            this->isShutdown = IMMEDIATE_SHUTDOWN;
        }
        // for that all thread may wait for new task, so i need to signal them
        pthread_cond_broadcast(&this->notify);
    }

    ~ThreadPool() {
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

        conditionDestroy(this->notify);
        mutexDestroy(this->taskMutex);
        mutexDestroy(this->threadArrayMutex);
    }

private:
    enum shutdown_t {
        RUNNING, GRACEFUL_SHUTDOWN, IMMEDIATE_SHUTDOWN
    };

    struct ThreadArgv {
        ThreadPool *const pool;
        const int *const sigToBlock;

        ThreadArgv(ThreadPool *p, const int *sigToBlock) : pool(p), sigToBlock(sigToBlock) {
            assert(p != nullptr);
            assert(sigToBlock != nullptr);
        }
    };

private:
    const size_t threadCnt;
    const int *const sigToBlock;

    /*
     * taskQue is protected by taskMutex
     */
    queue<task_t> taskQue;

    /*
     * threadArray are use only for destructor to join all threads
     *
     * in order to make other thread see the state of this vector,
     * I should use memory barrier, so now i use lock to protect this vector.
     *
     * TODO should I use lock for memory barrier? a better way?
     */
    vector<pthread_t> threadArray;

    /*
     * This vector is to store the arg that pass to worker threads.
     *
     * for that I don't ready know pthread's memory model,
     * so I don't know whether save in this vector will safely public
     * the completely construct ThreadArgv object to another thread.
     * (about publication, see the Java concurrency in practice 3.2).
     *
     * So I need an lock to act as memory barrier.
     * However, the pthread can only accept static function,
     * so this lock must be global(static global) variable
     *
     * TODO find a better way
     */
    vector<ThreadArgv> argvArray;

    pthread_mutex_t taskMutex{};
    pthread_mutex_t threadArrayMutex{};

    /*
     * this condition is signal when one task is add to taskQue.
     * this condition is broadcast when shutdown the threadPool.
     *
     * this cond's lock is taskMutex.
     */
    pthread_cond_t notify{};

    atomic<int> runningThreadCnt{};
    atomic<int> startedThreadCnt{};
    atomic<shutdown_t> isShutdown{};

private:

    // TODO add a new thread when a thread exit
    static void *worker(void *argv) {
        /*
         * if the shutdown is called, then the thread wait at cond will response immediately,
         * the thread that run task will run until the task end and response immediately
         */

        // act as memory barrier;
        mutexLock(argvArrayMutex);
        ThreadArgv &threadArgv = *(ThreadArgv *) argv;
        mutexUnlock(argvArrayMutex);

        ThreadPool &pool = *(threadArgv.pool);
        const int *const p = threadArgv.sigToBlock;

        changeThreadSigMask(p, SIG_SETMASK);
        while (true) {
            // use timeout to avoid dead lock
            if (!mutexLock(pool.taskMutex, MUTEX_TIMEOUT_SEC)) {
                bug("ThreadPool::worker, there may be an dead lock on taskMutex");
            }
            // when return from cond wait, should check whether the cond is still true
            while (pool.taskQue.empty() && pool.isShutdown.load() == RUNNING) {
                if (!conditionWait(pool.notify, pool.taskMutex)) {
                    // if cond wait fail, exit this thread
                    mutexUnlock(pool.taskMutex);
                    atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
                    return nullptr;
                }
            }
            // for that only the shutdown func call broadcast,
            // so if taskQue is empty, and the conditionWait return true,
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

    static size_t computeThreadCnt(size_t threadCnt) {
        if (maxThreadCnt() < threadCnt) {
            return DEFAULT_THREAD_CNT;
        }
        return threadCnt;
    }
};
