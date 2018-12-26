/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    threadPool.h
*   Author:       H-ZeX
*   Create Time:  2018-08-23-15:12:49
*   Describe:
*
**********************************************************************************/

#include "def.h"
#include "threadUtility.h"
#include "utility.h"
#include <atomic>
#include <pthread.h>
#include <queue>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
using namespace std;

class ThreadPool {
  public:
    /**
     * @param sigToBlock should end with 0, or there may be buffer overload
     * although set the main thread's mask and its child thread will inherit
     * to keep the modularization of ThreadPool, I still reserve this param
     */
    ThreadPool(size_t threadCnt = DEFAULT_THREAD_CNT, const int *sigToBlock = nullptr);
    /**
     * @return fail the taskQue is full or had call shutdown
     */
    bool addTask(const task_t &task);
    /**
     * @param completeRest whether compelete the rest of task
     */
    void shutdown(bool completeRest = true);
    ~ThreadPool();

  private:
    enum shutdown_t { noShutdown, gracefulShutdown, immediateShutdown };
    struct ThreadArgv {
        ThreadPool *pool;
        const int *sigToBlock;
        ThreadArgv() : pool(nullptr) , sigToBlock(nullptr){}
        ThreadArgv(ThreadPool *p, const int *siga) : pool(p), sigToBlock(siga) {}
    };

  private:
    queue<task_t> taskQue;
    vector<pthread_t> threadArray;
    vector<ThreadArgv> argvArray;
    pthread_mutex_t taskMutex;
    pthread_cond_t notify;
    atomic<int> runningThreadCnt;
    atomic<int> startedThreadCnt;
    atomic<shutdown_t> isShutdown;

  private:
    static void *worker(void *argv);
};
