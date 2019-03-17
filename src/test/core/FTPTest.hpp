//
// Created by root on 19-3-17.
//

#include "src/main/core/FTP.hpp"
#include "config.hpp"

class FTPTest {
public:
    static void test1() {
        FTP *ftp = FTP::getInstance(CORE_TEST_FTP_LISTEN_PORT);
        ftp->startAndRun();
    }

    // static void test5() {
    //     int listen = openListenFd("8084", 20);
    //     assert(listen >= 3);
    //     while (true);
    // }

    static void test2() {
        int listen = openListenFd("8033");
        assert(listen >= 3);
        ThreadPool *pool = ThreadPool::getInstance(1024);
        vector<int> fds;
        while (true) {
            int fd = acceptConnect(listen);
            assert(fd >= 3);
            fds.push_back(fd);
            pool->addTask(Task(handler, &*fds.rbegin()));
        }
    }

    static void test3() {
        const int maxFdCnt = 1024 * 1024;
        int *fds = new int[maxFdCnt];
        auto *pids = new pthread_t[maxFdCnt];
        int listen = openListenFd("8085", 20);
        int ind = 0;
        while (true) {
            int fd = acceptConnect(listen);
            printf("%s\n", ("accept one connection: " + to_string(fd)).c_str());
            fflush(stdout);
            assert(fd >= 3);

            fds[ind] = fd;
            assert(createThread(pids[ind], handler, &fds[ind]));
            ind++;
            if (ind >= maxFdCnt) {
                break;
            }
        }
        delete[] pids;
        delete[]fds;
    }

private:
    static void *handler(void *argv) {
        int fd = *(int *) argv;
        assert(fd >= 3);
        string msg = "hello\r\n";

        assert(write(fd, msg.c_str(), msg.length()) == msg.length());
        // assert(writeAllData(fd, msg.c_str(), msg.length()));
        closeFileDescriptor(fd);

        printf("%s\n", ("close one connection: " + to_string(fd)).c_str());
        fflush(stdout);
        pthread_exit(nullptr);
    }
};
