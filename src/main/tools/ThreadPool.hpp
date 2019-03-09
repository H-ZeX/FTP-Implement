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
 * Important!
 * poolPtr MUST be init in `start` func, to avoid the escape of `this` point.
 * MUST NOT modify poolPtr and sigToBlock after they are init,
 * except in the destructor
 *
 * TODO find a better way
 */
const static int MAX_SIG_CNT = 30;
static pthread_mutex_t argvMutex = PTHREAD_MUTEX_INITIALIZER;
static ThreadPool *poolPtr;
static int sigToBlock[MAX_SIG_CNT + 1]{};

/*
 * The var below is for singleton.
 *
 * Should NOT use the poolPtr above,
 * the poolPtr only be inited after call `start` method
 *
 * TODO find a better way
 */
static ThreadPool *singleton = nullptr;
static pthread_mutex_t singletonMutex = PTHREAD_MUTEX_INITIALIZER;

struct Task {
    void *(*func)(void *){};

    void *argument{};

    void (*callback)(void *){};

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
     * After use this threadPool should delete this pointer.
     * However, MUST make sure that only delete it one time!
     */
    // TODO find a better way to free this object
    static ThreadPool *getInstance(size_t threadCnt = DEFAULT_THREAD_CNT,
                                   const int *sigToBlock = nullptr) {
        // MUST NOT use DCL: check whether singleton==nullptr before lock mutex.
        // In Java, this may make thread find an incomplete singleton. (JCIP ch16)
        // I don't know what will happen on pthread's memory model,
        // but lock the mutex is a safe way.
        mutexLock(singletonMutex);
        if (singleton == nullptr) {
            singleton = new ThreadPool(threadCnt, sigToBlock);
        }
        mutexUnlock(singletonMutex);
        return singleton;
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
        if (state != NEW) {
            // TODO replace this using exception
            bug("call ThreadPool::start after start it", true);
        }

        // the mutex act as memory barrier
        mutexLock(argvMutex);
        ::poolPtr = this;
        mutexUnlock(argvMutex);

        this->state = RUNNING;
        // the mutex act as memory barrier
        mutexLock(threadArrayMutex);
        for (size_t i = 0; i < threadCnt; i++) {
            pthread_t tid;
            if (createThread(tid, &(ThreadPool::worker), nullptr)) {
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
        // Will the task object incomplete construct?
        // the answer is Maybe.
        // if user construct an Task object C in thread A
        // and call this method with object C in thread B.
        // then this object C may be incomplete construct.
        // however, this is NOT our responsibility.

        if (!task.isValid()) {
            // TODO replace this using exception
            bug("pass invalid task to ThreadPool::addTask", true);
        }

        // before check the state, must lock the mutex.
        // or there may be have race condition:
        // one thread call shutdown while another thread call addTask.
        //
        // TODO maybe we can tolerate some inconsistency and improve the performance,
        //  this need more proof
        mutexLock(taskQueAndStateMutex);
        if (this->state == NEW) {
            // TODO use exception to replace this
            bug("call ThreadPool::addTask before start it");
        } else if (this->state != RUNNING) {
            // TODO replace this using exception
            bug("try to add task after shutdown this thread pool", true);
        }
        if (taskQue.size() >= MAX_TASK_CNT) {
            warning("ThreadPool::addTask taskQue is full");
            mutexUnlock(this->taskQueAndStateMutex);
            return false;
        } else {
            // the unlock call MUST after signal call.
            // queue will call copy constructor of Task.
            taskQue.push(task);
            pthread_cond_signal(&this->notify);
            mutexUnlock(taskQueAndStateMutex);
            return true;
        }
    }

    /**
     * @param completeRest whether complete the rest of task.
     * this method is thread safe
     */
    void shutdown(bool completeRest = true) {
        // MUST lock the mutex before change the state,
        // or while change state, there may be other thread addTask.
        mutexLock(taskQueAndStateMutex);
        if (completeRest) {
            this->state = GRACEFUL_SHUTDOWN;
        } else {
            this->state = IMMEDIATE_SHUTDOWN;
        }
        mutexUnlock(taskQueAndStateMutex);
        // For that there may be threads wait for new task,
        // so I need to signal them.
        pthread_cond_broadcast(&this->notify);
    }

    ~ThreadPool() {
        this->shutdown();

        // the mutex act as memory barrier
        mutexLock(threadArrayMutex);
        for (pthread_t i : threadArray) {
            joinThread(i);
        }
        mutexUnlock(threadArrayMutex);

        conditionDestroy(this->notify);
        mutexDestroy(this->taskQueAndStateMutex);
        mutexDestroy(this->threadArrayMutex);
        mutexDestroy(::argvMutex);
        mutexDestroy(::singletonMutex);
        ::poolPtr = nullptr;
        ::singleton = nullptr;
    }

private:
    /**
     * @param sigToBlock Should end with 0, or there may be buffer overload.
     * The max len of this array is 30.
     * <br/>
     * The thread inside pool will only block signal on sigToBlock.
     */
    explicit ThreadPool(size_t threadCnt = DEFAULT_THREAD_CNT,
                        const int *sigToBlock = nullptr)
            : threadCnt(computeThreadCnt(threadCnt)) {
        // make sure that the `this` point is NOT escaped
        startedThreadCnt = 0;
        state = NEW;
        if (!conditionInit(notify)) {
            warning("init threadPool failed");
            return;
        }
        threadArray.reserve(threadCnt);
        copySigToBlockArray(sigToBlock);
    }

    void copySigToBlockArray(const int *sigToBlock) const {
        // the mutex act as memory barrier
        mutexLock(argvMutex);
        if (sigToBlock == nullptr) {
            ::sigToBlock[0] = 0;
            mutexUnlock(argvMutex);
            return;
        }
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

    // TODO add a new thread when a thread exit
    static void *worker(void *argv) {
        /*
         * if the shutdown is called,
         * then the thread wait at cond will response immediately,
         * the thread that run task will run until the task end
         * and response immediately
         */

        (void) argv;

        // act as memory barrier;
        mutexLock(argvMutex);
        ThreadPool &pool = *::poolPtr;
        changeThreadSigMask(::sigToBlock, SIG_SETMASK);
        mutexUnlock(argvMutex);

        if (pool.state == NEW) {
            bug("ThreadPool::worker should be run when the pool's state is NOT NEW");
        }
        while (true) {
            mutexLock(pool.taskQueAndStateMutex);
            // when return from cond wait, should check whether the cond is still true.
            // if stat is gracefulShutdown, should not wait here.
            while (pool.taskQue.empty() && pool.state.load() == RUNNING) {
                conditionWait(pool.notify, pool.taskQueAndStateMutex);
            }
            // for that only the shutdown func call broadcast,
            // so if taskQue is empty, and the conditionWait return true,
            // one of these check will success
            if ((pool.state.load() == IMMEDIATE_SHUTDOWN) ||
                (pool.state.load() == GRACEFUL_SHUTDOWN && pool.taskQue.empty())) {
                mutexUnlock(pool.taskQueAndStateMutex);
                atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
                return nullptr;
            }

            const Task task = pool.taskQue.front();
            pool.taskQue.pop();
            mutexUnlock(pool.taskQueAndStateMutex);

            void *r = task.func(task.argument);
            if (task.callback) {
                task.callback(r);
            }
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
    enum State {
        // if change this state enum, then MUST check the program carefully
                NEW, RUNNING, GRACEFUL_SHUTDOWN, IMMEDIATE_SHUTDOWN
    };

private:
    const size_t threadCnt;

    /*
     * taskQue is protected by taskQueAndStateMutex
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

    pthread_mutex_t taskQueAndStateMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t threadArrayMutex = PTHREAD_MUTEX_INITIALIZER;

    /*
     * this condition is signal when one task is add to taskQue.
     * this condition is broadcast when shutdown the threadPool.
     *
     * this cond's lock is taskQueAndStateMutex.
     */
    pthread_cond_t notify{};

    atomic<int> startedThreadCnt{};
    // the state's transformation:
    // NEW->RUNNING->GRACEFUL_SHUTDOWN or IMMEDIATE_SHUTDOWN
    atomic<State> state{};
};
