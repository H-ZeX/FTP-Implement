#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "src/main/util/Def.hpp"
#include "src/main/util/ThreadUtility.hpp"
#include "src/main/util/Utility.hpp"
#include "src/main/config/config.hpp"
#include <atomic>
#include <queue>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

class ThreadPool;

const static int MAX_SIG_CNT = 30;
/*
 * I don't know if use point to pass these var to the runner,
 * will the var that init in control thread visible to the new thread?
 * So I need a mutex to act as memory barrier.
 * And, as I use the global var, so this class should only be singleton.
 * So it is ok to let the var that pass to runner be global.
 * TODO find a better way
 *
 * This two var is effective constant.
 * The sigToBlock and poolPtr is protected by poolPtrAndSigToBlockMutex.
 *
 * Important!
 * poolPtr MUST be init in `start` func, to avoid the escape of `this` point.
 * MUST NOT modify poolPtr and sigToBlock after they are init,
 * except in the destructor.
 *
 */
static pthread_mutex_t poolPtrAndSigToBlockMutex = PTHREAD_MUTEX_INITIALIZER;
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
    void *(*runnable)(void *) = nullptr;

    void (*callback)(void *) = nullptr;

    void *argument = nullptr;

    explicit Task(void *(*runnable)(void *),
                  void *argument = nullptr,
                  void (*callback)(void *) = nullptr) {
        this->runnable = runnable;
        this->argument = argument;
        this->callback = callback;
    }

    bool isValid() const {
        return this->runnable != nullptr;
    }
};

/**
 * This threadPool is a fix threadPool.
 * <br/>
 * The public method inside this class is thread safe.
 * This ThreadPool should be singleton in the whole application.
 */
class ThreadPool {
public:

    /**
     * @note
     * After use this threadPool should delete this pointer.
     * In the destructor, will block until all work thread end.
     * <br/>
     * @warning
     * MUST make sure that only delete it one time!
     * <br/>
     * If call other method (except this static getInstance)
     * while another thread is delete the ThreadPool object,
     * the behavior is Undefined!
     * <br/>
     * After delete the instance, and call getInstance,
     * it will return a new ThreadPool instance.
     *
     * @param sigToBlock This array should end with nullptr.
     * The max len of this array is 31(include the nullptr).
     * The thread inside pool will only block signal on sigToBlock.
     * @param threadCnt if it is larger that the maxThreadCnt(the OS's limit),
     * it will replace by DEFAULT_THREAD_CNT.
     */
    // TODO find a better way to free this object
    static ThreadPool *getInstance(size_t threadCnt = DEFAULT_THREAD_CNT,
                                   const int *sigToBlock = nullptr) {
        /*
         * MUST NOT use DCL: check whether singleton==nullptr before lock mutex.
         * In Java, this may make thread find an incomplete singleton. (JCIP ch16)
         * I don't know what will happen on pthread's memory model,
         * but lock the mutex is a safe way.
         */
        mutexLock(singletonMutex);
        if (singleton == nullptr) {
            singleton = new ThreadPool(threadCnt, sigToBlock);
        }
        mutexUnlock(singletonMutex);
        return singleton;
    }

    /**
     * start this threadPool.
     * call it after it had been called has no effect.
     */
    void start() {
        /*
         * this method is to createThread and init the global poolPtr.
         * according to Java concurrency in practice,
         * should NOT start thread in constructor, or the `this` point will escape.
         * which may make another thread see the incomplete construct object.
         */

        /*
         * If multiple thread call this start method,
         * then there will only one thread get the mutex
         * and init the state, create threads success,
         * others will return.
         * For any instance, only the getInstance call the constructor once,
         * so if the state is not NEW, it will never be NEW.
         */
        mutexLock(taskQueAndStateMutex);
        if (state == GRACEFUL_SHUTDOWN || state == IMMEDIATE_SHUTDOWN) {
            // TODO replace it using exception
            bug("MUST NOT call ThreadPool::start after shutdown it", true);
        }
        if (state != NEW) {
            mutexUnlock(taskQueAndStateMutex);
            return;
        }
        this->state = RUNNING;
        mutexUnlock(taskQueAndStateMutex);

        // this sleep is help to test addTask before the threads is complete created
        // usleep(10000);

        // the mutex act as memory barrier
        mutexLock(poolPtrAndSigToBlockMutex);
        ::poolPtr = this;
        mutexUnlock(poolPtrAndSigToBlockMutex);

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
     * @note
     * The first time call this method will lead to start this ThreadPool.
     * Which will create the work threads.
     * If you want to create the threads before addTask,
     * call the start method beforehand.
     * <br/>
     * This method grantee that the Task object will safely published to the worker thread.
     * Which means that the work thread will see the completely Task object.
     * <br/>
     * However, user MUST make sure NOT to pass an Task object which is constructed in another thread
     * to this method (the thread that construct the object different from the thread that call this method)
     * without enough synchronization.
     * @warning
     * Should NOT addTask after call shutdown method.
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
        if (state == GRACEFUL_SHUTDOWN || state == IMMEDIATE_SHUTDOWN) {
            // TODO replace it using exception
            bug("MUST NOT call ThreadPool::start after shutdown it", true);
        }
        if (this->state == NEW) {
            mutexUnlock(taskQueAndStateMutex);
            start();
            mutexLock(taskQueAndStateMutex);
        }
        if (taskQue.size() >= THREAD_POOL_MAX_TASK_CNT) {
            warning("ThreadPool::addTask taskQue is full");
            mutexUnlock(this->taskQueAndStateMutex);
            return false;
        } else {
            // the unlock call MUST after signal call.
            // Note that queue will call copy constructor of Task,
            taskQue.push(task);
            conditionSignal(this->taskQueCondAndShutdownNotify);
            mutexUnlock(taskQueAndStateMutex);
            return true;
        }
    }

    /**
     * @param completeRest whether complete the rest of task.
     * @warning
     * After call this shutdown, should NOT call other method.
     * Except delete it and call getInstance to get a new instance.
     * @note
     * Call this shutdown after one thread had call it has no effect.
     */
    void shutdown(bool completeRest = true) {
        // MUST lock the mutex before change the state,
        // or while change state, there may be other thread addTask.
        //
        // Make sure that, after finish this shutdown method,
        // call other method(except destructor and getInstance) will fail.
        mutexLock(taskQueAndStateMutex);
        if (completeRest) {
            this->state = GRACEFUL_SHUTDOWN;
        } else {
            this->state = IMMEDIATE_SHUTDOWN;
        }
        mutexUnlock(taskQueAndStateMutex);
        // For that there may be threads wait for new task,
        // so I need to signal them.
        conditionBroadcast(this->taskQueCondAndShutdownNotify);
    }

    ~ThreadPool() {
        // MUST make sure when call this destructor,
        // the getInstance is NOT called.
        // So this destructor should lock the singletonMutex.
        //
        // Needn't to worry user call other method while destructor running,
        // user's such behavior is illegal.
        //
        // Needn't to destroy this poolPtrAndSigToBlockMutex and singletonMutex,
        // it is safety to use it in multiple instance.

        mutexLock(singletonMutex);
        // when test, should test delete the object without call shutdown before.
        // To test whether the thread handler the condition correctly
        this->shutdown();

        // the mutex act as memory barrier
        mutexLock(threadArrayMutex);
        for (pthread_t i : threadArray) {
            joinThread(i);
        }
        mutexUnlock(threadArrayMutex);

        // the mutex act as memory barrier
        mutexLock(poolPtrAndSigToBlockMutex);
        ::poolPtr = nullptr;
        memset(sigToBlock, 0, MAX_SIG_CNT + 1);
        mutexUnlock(poolPtrAndSigToBlockMutex);

        // we had lock singletonMutex above
        ::singleton = nullptr;

        conditionDestroy(this->taskQueCondAndShutdownNotify);
        mutexDestroy(this->taskQueAndStateMutex);
        mutexDestroy(this->threadArrayMutex);

        mutexUnlock(singletonMutex);
    }

private:
    /**
     * @param sigToBlock Should end with 0, or there may be buffer overload.
     * The max len of this array is 30.
     * <br/>
     * The thread inside pool will only block signal on sigToBlock.
     */
    /*
     * only the getInstance called this constructor,
     * the getInstance is protected by singletonMutex.
     */
    explicit ThreadPool(size_t threadCnt = DEFAULT_THREAD_CNT,
                        const int *sigToBlock = nullptr)
            : threadCnt(computeThreadCnt(threadCnt)) {
        // make sure that the `this` point is NOT escaped
        startedThreadCnt = 0;
        state = NEW;
        if (!conditionInit(taskQueCondAndShutdownNotify)) {
            warning("init threadPool failed");
            return;
        }
        threadArray.reserve(threadCnt);
        copySigToBlockArray(sigToBlock);
    }

    void copySigToBlockArray(const int *sigToBlock) const {
        // the mutex act as memory barrier
        mutexLock(poolPtrAndSigToBlockMutex);
        if (sigToBlock == nullptr) {
            ::sigToBlock[0] = 0;
            mutexUnlock(poolPtrAndSigToBlockMutex);
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
        mutexUnlock(poolPtrAndSigToBlockMutex);
    }

    static void *worker(void *argv) {
        /*
         * if the shutdown is called,
         * then the thread wait at cond will response immediately,
         * the thread that run task will run until the task end
         * and response immediately
         */

        (void) argv;

        // act as memory barrier;
        mutexLock(poolPtrAndSigToBlockMutex);
        ThreadPool &pool = *::poolPtr;
        changeThreadSigMask(::sigToBlock, SIG_SETMASK);
        mutexUnlock(poolPtrAndSigToBlockMutex);

        if (pool.state == NEW) {
            bug("ThreadPool::worker should be run when the pool's state is NOT NEW");
        }
        while (true) {
            mutexLock(pool.taskQueAndStateMutex);
            // when return from cond wait, should check whether the cond is still true.
            // if stat is gracefulShutdown, should not wait here.
            while (pool.taskQue.empty() && pool.state == RUNNING) {
                conditionWait(pool.taskQueCondAndShutdownNotify, pool.taskQueAndStateMutex);
            }
            // for that only the shutdown func call broadcast,
            // so if taskQue is empty, and the conditionWait return true,
            // one of these check will success
            if ((pool.state == IMMEDIATE_SHUTDOWN) ||
                (pool.state == GRACEFUL_SHUTDOWN && pool.taskQue.empty())) {
                mutexUnlock(pool.taskQueAndStateMutex);
                atomic_fetch_sub<int>(&(pool.startedThreadCnt), 1);
                return nullptr;
            }

            const Task task = pool.taskQue.front();
            pool.taskQue.pop();
            mutexUnlock(pool.taskQueAndStateMutex);

            // TODO
            //  As we use the taskQueAndStateMutex,
            //  Will we see the complete object that passed by user?
            void *r = task.runnable(task.argument);
            if (task.callback) {
                task.callback(r);
            }
            if ((pool.state == IMMEDIATE_SHUTDOWN)) {
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
        // if change this state enum, then MUST check the program carefully.
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
    pthread_cond_t taskQueCondAndShutdownNotify{};

    atomic<int> startedThreadCnt{};
    /*
     * the state's transformation:
     * NEW->RUNNING->GRACEFUL_SHUTDOWN or IMMEDIATE_SHUTDOWN
     * or
     * NEW->GRACEFUL_SHUTDOWN or IMMEDIATE_SHUTDOWN

     * In the constructor, the state is set to NEW.
     * In the start method, the state is set to RUNNING.
     * In the shutdown method, the state set to GRACEFUL_SHUTDOWN or IMMEDIATE_SHUTDOWN.

     * proof of the state transformation safety.
     * For any instance of this ThreadPool,
     * the constructor run only one time,
     * so for any instance, if it is not NEW state,
     * it will never be NEW state.
     * After the shutdown method is called,
     * no other public method except destructor and getInstance can be called,
     * so if it is shutdown state, it will always be shutdown state.
     * Every time the state is modify, it is protected by taskQueAndStateMutex,
     * so there is no thread safety problem.
    */
    atomic<State> state{};
};

#endif
