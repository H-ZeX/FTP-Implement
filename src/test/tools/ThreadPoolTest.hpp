//
// Created by hzx on 19-3-9.
//

#ifndef FTP_SERVER_TOOLS_TEST_H
#define FTP_SERVER_TOOLS_TEST_H


#include <src/main/tools/ThreadPool.hpp>

#include "src/test/util/UtilityTest.hpp"

// testThreadPoolV1 param
static atomic_int addResultV1{};

// testThreadPoolV1 param
static atomic_int addResultV2{};
static atomic_int finishCntV2{};

class ThreadPoolTest {
public:
    static void testThreadPoolV1() {
        srand(static_cast<unsigned int>(clock()));

        ThreadPool *pool = ThreadPool::getInstance();
        pool->start();
        addResultV1 = 0;

        const int taskCnt = 100;
        int data[taskCnt];
        for (int &j : data) {
            j = 1;
            // TODO when the worker thread see &j(the ptr),
            //  will it see correct j itself.
            pool->addTask(Task((ThreadPoolTest::runnerV1), &j));
        }
        pool->shutdown();
        delete pool;
        if (addResultV1 != taskCnt) {
            cout << addResultV1 << " " << taskCnt << endl;
        }
        assert(addResultV1 == taskCnt);
    }

    static void testThreadPoolV2() {
        const int testCnt = 1024;
        const int maxTaskCnt = 100;
        const int maxThreadCnt = 100;
        srand(static_cast<unsigned int>(clock()));
        for (int i = 0; i < testCnt; i++) {
            addResultV2 = 0;
            finishCntV2 = 0;
            const int threadCnt = static_cast<const int>(random() % maxThreadCnt);
            const int taskCnt = static_cast<const int>(random() % maxTaskCnt);
            pthread_t threads[threadCnt];
            for (int j = 0; j < threadCnt; j++) {
                // TODO will taskCnt safely publish to new thread
                bool success = createThread(threads[j], ThreadPoolTest::addTaskRunner, (void *) &taskCnt);
                if (!success) {
                    fprintf(stderr, "This test create too many threads!");
                    exit(0);
                }
            }
            // If I use join but not the way below(has been commented),
            // then such maxThreadCnt config will not lead to create thread failed.
            // However, the way below will lead to create failed.
            // Is the pthread_join clear the resource? (I did not find this in manual).
            for (int k = 0; k < threadCnt; k++) {
                joinThread(threads[k]);
            }
            // while (finishCntV2 != threadCnt) {
            //     pthread_yield();
            // }
            // delete after all task has ended
            delete ThreadPool::getInstance();

            const int want = threadCnt * (taskCnt - 1) * taskCnt / 2;
            if (want != addResultV2) {
                cout << want << " " << addResultV2 << endl;
            }
            assert(addResultV2 == want);
            cout << "success " << i << endl;
        }
    }

    static void testThreadPoolV3() {
        ThreadPool *pool = ThreadPool::getInstance();
        const int testCnt = 5;
        for (int i = 0; i < testCnt; i++) {
            pool->addTask(Task(ThreadPoolTest::runnerV3));
        }
        delete pool;
    }

private:
    /**
     * add 0...taskCnt-1 to the addResultV2 using ThreadPool
     */
    static void *addTaskRunner(void *argv) {
        const int taskCnt = *(int *) argv;
        ThreadPool &pool = *ThreadPool::getInstance(100);
        // wait other thread
        usleep(static_cast<__useconds_t>(random() % 1000));
        // should test when this statement is commented or not.
        // pool.start();

        atomic_int endCnt{};
        Argv argvArray[taskCnt];
        for (int i = 0; i < taskCnt; i++) {
            argvArray[i].toAdd = i;
            argvArray[i].endCnt = &endCnt;
            // the pool.addTask will safely publish the Task object to worker thread
            // TODO when worker thread see the endCnt ptr, will it see a complete endCnt object
            pool.addTask(Task(ThreadPoolTest::runnerV2, &argvArray[i]));
        }
        while (endCnt != taskCnt) {
            pthread_yield();
        }
        finishCntV2 += 1;
        return nullptr;
    }

private:
    struct Argv {
        int toAdd = 0;
        atomic_int *endCnt = nullptr;
    };

    static void *runnerV1(void *param) {
        int toAdd = *(int *) param;
        usleep(static_cast<__useconds_t>(random() % 1000));
        addResultV1 += toAdd;
        return nullptr;
    }

    static void *runnerV2(void *param) {
        Argv &argv = *(Argv *) param;
        // sleep to wait other thread
        usleep(static_cast<__useconds_t>(random() % 1000));
        atomic_fetch_add(&addResultV2, argv.toAdd);
        atomic_fetch_add((argv.endCnt), 1);
        return nullptr;
    }

    static void *runnerV3(void *param) {
        (void) param;
        UtilityTest::testOpenListenFd(10);
        return nullptr;
    }
};

#endif //FTP_SERVER_TOOLS_TEST_H
