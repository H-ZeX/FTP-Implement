//
// Created by root on 19-3-17.
//

#include "src/main/core/FTP.hpp"

class FTPTest {
public:
    static void test1() {
        FTP *ftp = FTP::getInstance(8001, 4);
        ftp->startAndRun();
    }

    static void test2() {
        int listen = openListenFd("8033");
        assert(listen >= 3);
        ThreadPool *pool = ThreadPool::getInstance(1024);
        vector<int> fds;
        while (true) {
            int fd = acceptConnect(listen);
            fds.push_back(fd);
            assert(fd >= 3);
            pool->addTask(Task(handler, &*fds.rbegin()));
        }
    }

    static void test3() {
        int epollFd = epoll_create(1);
        if (epollFd < 0) {
            warningWithErrno("FTP::startAndRun epoll_create", errno);
            exit(0);
        }
        ThreadPool *pool = ThreadPool::getInstance();
        int listen = openListenFd("8089", 1024);
        addToEpollInterestList(listen, epollFd);
        epoll_event ev[1024];
        int ind = 0;
        const int maxFdCnt = 1024 * 1024;
        int *fds = new int[maxFdCnt];
        while (true) {
            // int ss = epollWaitWrap(epollFd, ev, 1024, -1);
            // assert(ss == 1);
            // for (int i = 0; i < ss; i++) {
            int fd = acceptConnect(listen);
            assert(fd >= 3);
            fds[ind] = fd;
            pool->addTask(Task(handler, &fds[ind]));
            ind++;
            // }
            if (ind >= maxFdCnt) {
                break;
            }
        }
        delete pool;
        delete[]fds;
    }

private:

    /**
 * @return whether success, when fail, the errno is set appropriately,
 * errno can be ENOMEM ENOSPC.
 */
    static bool addToEpollInterestList(int fd, int epollFd, uint32_t event = EPOLLIN) {
        epoll_event ev{};
        ev.events = event;
        ev.data.fd = fd;
        return epollCtlWrap(epollFd, EPOLL_CTL_ADD, fd, &ev);
    }


    static void *handler(void *argv) {
        int fd = *(int *) argv;
        string msg = "hello\r\n";
        assert(writeAllData(fd, msg.c_str(), msg.length()));
        printf("%s\n", ("accept one connection: " + to_string(fd)).c_str());
        fflush(stdout);
        closeFileDescriptor(fd);
        printf("%s\n", ("close one connection: " + to_string(fd)).c_str());
        fflush(stdout);
        return nullptr;
    }
};
