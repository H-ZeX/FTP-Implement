//
// Created by hzx on 19-3-9.
//

#ifndef FTP_SERVER_TOOLS_TEST_H
#define FTP_SERVER_TOOLS_TEST_H


#include <src/main/tools/ThreadPool.hpp>

static atomic_int cnt{};
static atomic_int finishCnt{};

class ToolsTest {
public:
    static void testThreadPoolV1() {
        srand(static_cast<unsigned int>(clock()));
        ThreadPool *pool = ThreadPool::getInstance();
        pool->start();
        const int taskCnt = 10240;
        int data[taskCnt];
        for (int j = 0; j < taskCnt; j++) {
            data[j] = 1;
            pool->addTask(Task((ToolsTest::runner), &data[j], nullptr));
        }
        pool->shutdown(false);
        delete pool;
        if (cnt != taskCnt) {
            cout << cnt << " " << taskCnt << endl;
        }
        assert(cnt == taskCnt);
    }

    static void testThreadPoolV2() {
        const int testCnt = 100;
        srand(static_cast<unsigned int>(clock()));
        for (int i = 0; i < testCnt; i++) {
            cnt = 0;
            finishCnt = 0;
            const int threadCnt = static_cast<const int>(random() % 10);
            int data[threadCnt];
            for (int j = 0; j < threadCnt; j++) {
                pthread_t pid;
                data[j] = j;
                createThread(pid, ToolsTest::addTaskRunner, &data[j]);
            }
            while (finishCnt != threadCnt) {
                assert(pthread_yield() == 0);
            }
            int want = threadCnt * (99 + 0) * 100 / 2;
            assert(threadCnt == 0 || want > 0);
            if (want != cnt) {
                cout << want << " " << cnt << endl;
            }
            delete ThreadPool::getInstance();
            assert(cnt == want);
            cout << "success " << i << endl;
        }
    }

    static void *addTaskRunner(void *argv) {
        (void) argv;
        ThreadPool &pool = *ThreadPool::getInstance(10);
        // wait other thread
        usleep(static_cast<__useconds_t>(random() % 1000));

        pool.start();
        atomic_int endCnt{};
        Argv a[100];
        for (int i = 0; i < 100; i++) {
            a[i].toAdd = i;
            a[i].endCnt = &endCnt;
            pool.addTask(Task(ToolsTest::runner, &a[i]));
        }
        while (endCnt != 100) {
            pthread_yield();
        }
        // cout << "one addTaskRunner end\n";
        atomic_fetch_add(&finishCnt, 1);
        return nullptr;
    }

private:
    struct Argv {
        int toAdd = 0;
        atomic_int *endCnt = nullptr;
    };

    static void *runner(void *argv) {
        Argv &a = *(Argv *) argv;
        int t = a.toAdd;
        // sleep to wait other thread
        usleep(static_cast<__useconds_t>(random() % 1000));

        assert(t < 100);
        int *p = new int[1024];
        for (int j = 0; j < 1024; j++) {
            p[j] = t;
        }
        int k = 0;
        for (int i = 0; i < 1024; i++) {
            k += p[i];
        }
        if (k / 1024 != t) {
            printf("k/1024=%d\tt=%d\n", k / 1024, t);
        }
        assert(k / 1024 == t);
        atomic_fetch_add(&cnt, k / 1024);
        delete[] p;
        atomic_fetch_add((a.endCnt), 1);
        return nullptr;
    }
};

#endif //FTP_SERVER_TOOLS_TEST_H
