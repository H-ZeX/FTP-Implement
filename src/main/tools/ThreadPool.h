/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    threadPool.h
*   Author:       H-ZeX
*   Create Time:  2018-08-23-15:12:49
*   Describe:
*
**********************************************************************************/

#include "src/main/util/def.h"
#include "src/main/util/ThreadUtility.h"
#include "src/main/util/utility.h"
#include <atomic>
#include <pthread.h>
#include <queue>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

/**
 * this class is thread safe
 */
class ThreadPool {
public:
    /**
     * @param sigToBlock should end with 0, or there may be buffer overload.
     * Although the child thread will inherit the main thread's mask,
     * in order to keep the modularization of this ThreadPool class,
     * I still reserve this param
     */
    explicit ThreadPool(size_t threadCnt = DEFAULT_THREAD_CNT, const int *sigToBlock = nullptr);

    /**
     * start this threadPool
     */
    void start();

    /**
     * @return whether addTask success,
     * will fail when the taskQue is full or had called shutdown.
     * this method is thread safe
     */
    bool addTask(const task_t &task);

    /**
     * @param completeRest whether complete the rest of task.
     * this method is thread safe
     */
    void shutdown(bool completeRest = true);

    ~ThreadPool();

private:
    enum shutdown_t {
        RUNNING, GRACEFUL_SHUTDOWN, IMMEDIATE_SHUTDOWN
    };

    struct ThreadArgv {
        ThreadPool *pool;
        const int *sigToBlock;

        ThreadArgv() : pool(nullptr), sigToBlock(nullptr) {}

        ThreadArgv(ThreadPool *p, const int *sigToBlock) : pool(p), sigToBlock(sigToBlock) {}
    };

private:
    const size_t threadCnt;
    const int *const sigToBlock;

    /**
     * taskQue is protected by taskMutex
     */
    queue<task_t> taskQue;

    /**
     * threadArray are use only for destructor to join all threads
     *
     * in order to make other thread see the state of this vector,
     * I should use memory barrier, so now i use lock to protect this vector
     *
     * TODO should I use lock for memory barrier?
     */
    vector<pthread_t> threadArray;
    /**
     * This vector is to store the arg that pass to worker threads.
     * for that I don't ready know pthread's memory model,
     * so I don't know whether save in this vector will safely public
     * the completely construct ThreadArgv object to another thread.
     * (about publication, see the Java concurrency in practice 3.2).
     *
     * TODO is the problem exist, and how to solve it
     */
    vector<ThreadArgv> argvArray;
    /**
     * notify is associate with taskMutex
     */
    pthread_mutex_t taskMutex;
    pthread_mutex_t threadArrayMutex;

    /**
     * this condition is signal when one task is add to taskQue.
     * this condition is broadcast when shutdown the threadPool.
     */
    pthread_cond_t notify;

    atomic<int> runningThreadCnt;
    atomic<int> startedThreadCnt;
    atomic<shutdown_t> isShutdown;

private:
    static void *worker(void *argv);

    static size_t computeThreadCnt(size_t threadCnt);
};
