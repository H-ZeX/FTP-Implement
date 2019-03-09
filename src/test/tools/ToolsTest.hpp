//
// Created by hzx on 19-3-9.
//

#ifndef FTP_SERVER_TOOLS_TEST_H
#define FTP_SERVER_TOOLS_TEST_H


#include <src/main/tools/ThreadPool.hpp>

static atomic_int cnt{};

class ToolsTest {
public:
    static void testThreadPool() {
        srand(static_cast<unsigned int>(clock()));
        ThreadPool *pool = ThreadPool::getInstance();
        pool->start();
        const int taskCnt = 1024;
        for (int j = 0; j < taskCnt; j++) {
            pool->addTask(Task(&(ToolsTest::runner)));
        }
        delete pool;
        assert(cnt == taskCnt);
    }

private:
    static void *runner(void *argv) {
        (void) argv;
        int *p = new int[1024];
        memset(p, 0x1, 1024);
        atomic_fetch_add(&cnt, 1);
        delete[] p;
        return nullptr;
    }
};


#endif //FTP_SERVER_TOOLS_TEST_H
