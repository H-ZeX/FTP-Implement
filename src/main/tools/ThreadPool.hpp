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
#include "src/main/util/Utility.hpp"
#include <atomic>
#include <pthread.h>
#include <queue>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <cassert>

using namespace std;

class ThreadPool;

/*
 * pthread's runner func should be static,
 * however, this two param must be pass to the runner.
 * so I use such global var.
 *
 * This two var is effective constant,
 * however, I don't know how pthread's memory model handle such var,
 * will it be visible to other thread after init by control thread ?
 * So I use an mutex, act as the memory barrier.
 *
 * Make sure that, the mutex and sigToBlock MUST be init by ThreadPool's constructor.
 * And poolPtr MUST be init in `start` func, to avoid the escape of `this` point.
 *
 * TODO find a better way
 */
static pthread_mutex_t argvMutex;
static ThreadPool *poolPtr;
static int *sigToBlock;

/*
 * This is used by signal handler an threadPool,
 * so it must be global
 */
static volatile bool willExit = false;


struct Task {
    void *(*func)(void *);

    void *argument;

    void (*callback)(void *);

    Task() {
        this->func = nullptr;
        this->argument = nullptr;
        this->callback = nullptr;
    }

    explicit Task(void *(*func)(void *),
                  void *argument = nullptr,
                  void (*callback)(void *) = nullptr) {
        this->func = func;
        this->argument = argument;
        this->callback = callback;
    }

    bool isValid() const {
        return this->func != nullptr;
    }
};

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
                        const int *sigToBlock = {})
            : threadCnt(computeThreadCnt(threadCnt)) {
        // make sure that the `this` point is NOT escaped
        runningThreadCnt = 0;
        startedThreadCnt = 0;
        state = NEW;
        if (!mutexInit(argvMutex)
            || !mutexInit(taskMutex)
            || !mutexInit(threadArrayMutex)
            || !conditionInit(notify)) {
            warning("init threadPool failed");
            return;
        }
        threadArray.reserve(threadCnt);
        copySigToBlockArray(sigToBlock);
    }

    void copySigToBlockArray(const int *sigToBlock) const {
        // act as memory barrier
        mutexLock(argvMutex);
        int ind = 0;
        while (ind < MAX_SIG_CNT && sigToBlock[ind]) {
            ::sigToBlock[ind] = sigToBlock[ind];
            ind++;
        }
        // TODO use exception to replace this
        if (sigToBlock[ind] != 0) {
            bug("ThreadPool's sigToBlock param is illegal");
        }
        mutexUnlock(argvMutex);
    }

    /**
     * start this threadPool
     */
    void start() {
        /*
         * this method is to createThread and init the global poolPtr.
         * according to Java concurrency in practice,
         * should NOT start thread in constructor, or the `this` point will escape.
         * which may make another thread see the incomplete construct object.
         */

        // act as memory barrier
        mutexLock(argvMutex);
        ::poolPtr = this;
        mutexUnlock(argvMutex);

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
     * It is illegal to call this method before call start method.
     * @return whether addTask success,
     * will fail when the taskQue is full or had called shutdown.
     */
    bool addTask(const Task &task) {
        if (this->state == NEW) {
            // TODO use exception to replace this
            bug("call ThreadPool::addTask before start it");
        }
        if (this->state != RUNNING) {
            return false;
        }

        // Will the task object incomplete construct?
        // the answer is Maybe.
        // if use construct an Task object in thread A
        // and call this method with this obj in thread B.
        // then this obj may be incomplete construct.
        // however, this is NOT our responsibility.

        if (!task.isValid()) {
            // TODO replace this using exception
            bug("pass invalid task to ThreadPool::addTask", true);
        }
        if (this->state.load() != RUNNING) {
            // TODO replace this using exception
            bug("try to add task after shutdown this thread pool" ,true);
        }
        mutexLock(taskMutex);
        if (taskQue.size() >= MAX_TASK_CNT) {
            warning("ThreadPool::addTask taskQue is full");
            mutexUnlock(this->taskMutex);
            return false;
        } else {
            // the unlock call MUST after signal call
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
            this->state = GRACEFUL_SHUTDOWN;
        } else {
            this->state = IMMEDIATE_SHUTDOWN;
        }
        // for that all thread may wait for new task, so i need to signal them
        pthread_cond_broadcast(&this->notify);
    }

    ~ThreadPool() {
        if (this->state.load() == RUNNING || this->runningThreadCnt.load() > 0 ||
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

    // TODO add a new thread when a thread exit
    static void *worker(void *argv) {
        /*
         * if the shutdown is called, then the thread wait at cond will response immediately,
         * the thread that run task will run until the task end and response immediately
         */

        // act as memory barrier;
        mutexLock(argvMutex);
        ThreadArgv &threadArgv = *(ThreadArgv *) argv;
        mutexUnlock(argvMutex);

        ThreadPool &pool = *(threadArgv.pool);
        changeThreadSigMask(threadArgv.sigToBlock, SIG_SETMASK);

        while (true) {
            mutexLock(pool.taskMutex);
            // when return from cond wait, should check whether the cond is still true.
            // if stat is gracefulShutdown, should not wait here.
            while (pool.taskQue.empty() && pool.state.load() == RUNNING) {
                conditionWait(pool.notify, pool.taskMutex);
            }
            // for that only the shutdown func call broadcast,
            // so if taskQue is empty, and the conditionWait return true,
            // one of these check will success
            if ((pool.state.load() == IMMEDIATE_SHUTDOWN) ||
                (pool.state.load() == GRACEFUL_SHUTDOWN && pool.taskQue.empty())) {
                mutexUnlock(pool.taskMutex);
                atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
                return nullptr;
            }

            const Task task = pool.taskQue.front();
            pool.taskQue.pop();
            mutexUnlock(pool.taskMutex);

            atomic_fetch_add<int>(&(pool.runningThreadCnt), 1);
            void *r = task.func(task.argument);
            if (task.callback) {
                task.callback(r);
            }
            atomic_fetch_sub<int>(&(pool.runningThreadCnt), 1);

            if ((pool.state.load() == IMMEDIATE_SHUTDOWN)) {
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


private:
    // TODO find a way to make this constant not static
    // it is use by sigToBlock array, so i need to be static
    const static int MAX_SIG_CNT = 30;
    enum state_t {
        NEW, RUNNING, GRACEFUL_SHUTDOWN, IMMEDIATE_SHUTDOWN
    };

    struct ThreadArgv {
        ThreadPool *const pool;
        int sigToBlock[MAX_SIG_CNT + 1];

        ThreadArgv(ThreadPool *p, const int *sigToBlock) : pool(p) {
            assert(p != nullptr);
            assert(sigToBlock != nullptr);
            int ind = 0;
            while (ind < MAX_SIG_CNT && sigToBlock[ind]) {
                this->sigToBlock[ind] = sigToBlock[ind];
                ind++;
            }
            assert(sigToBlock[ind] == 0);
        }
    };

private:
    const size_t threadCnt;

    /*
     * taskQue is protected by taskMutex
     */
    queue<Task> taskQue;

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
     */
    vector<ThreadArgv> argvArray;
    int sigToBlock[MAX_SIG_CNT + 1];

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
    atomic<state_t> state{};
};
