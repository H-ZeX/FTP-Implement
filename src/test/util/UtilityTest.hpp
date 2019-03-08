//
// Created by hzx on 19-3-8.
//

#ifndef FTP_SERVER_UTILITY_TEST_H
#define FTP_SERVER_UTILITY_TEST_H

#include <cassert>
#include "src/main/util/utility.hpp"
#include "src/main/util/ThreadUtility.hpp"
#include "src/main/util/NetUtility.hpp"


class UtilityTest {
public:
    static void testMereString() {
        char dest[1024];
        char *s = mereString(dest, (const char *const[]) {"abc", "", "", nullptr}, 1024);
        assert(string(s) == string(s));
        s = mereString(dest, (const char *const[]) {"", "", "", nullptr}, 1024);
        assert(string(s).empty());
        s = mereString(dest, (const char *const[]) {"", "dded", "e", nullptr}, 1024);
        assert(string(s)=="ddede");
        cout << "MereString Test Success" << endl;
    }

    static void testNumLen() {
        assert(numLen(100, 10) == 3);
        assert(numLen(10, 10) == 2);
        assert(numLen(1, 10) == 1);
        assert(numLen(-100, 10) == 3);
        assert(numLen(-10, 10) == 2);
        assert(numLen(-1, 10) == 1);
        assert(numLen(0, 10) == 0);
        cout << "NumLen Test Success" << endl;
    }

    static void testIO() {
        const int testCnt = 1024;
        const int maxSize = 1024;
        const int sizePerTimes = 100;
        int fd[2];
        srand(clock());
        assert(pipe(fd) == 0);
        for (int i = 0; i < testCnt; i++) {
            const auto size = static_cast<const size_t>(random() % maxSize);
            char data[size];
            for (int j = 0; j < size; j++) {
                data[j] = static_cast<char>(random() % 256);
            }
            char buf[size];
            size_t rest = size;
            ReadBuf buf1;
            while (rest > 0) {
                auto toWrite = static_cast<size_t>(random() % sizePerTimes);
                // cout << "toWrite: " << toWrite << endl;
                toWrite = min(toWrite, rest);
                assert(writeAllData(fd[1], data + size - rest, toWrite));
                {
                    int hadRead = 0;
                    while (hadRead < toWrite) {
                        int r = readWithBuf(fd[0], buf + size - rest + hadRead, toWrite - hadRead, buf1);
                        // cout << "Read: " << r << endl;
                        assert(r > 0);
                        hadRead += r;
                    }
                }
                // {
                //     size_t t = size - rest;
                //     for (int j = 0; j < toWrite; j++) {
                //         if (data[j + t] != buf[j + t]) {
                //             cout << t + j << endl;
                //             output(data, 0, t + toWrite);
                //             output(buf, 0, t + toWrite);
                //             assert(false);
                //         }
                //     }
                // }
                rest -= toWrite;
                // cout << endl;
            }
            for (int k = 0; k < size; k++) {
                if (buf[k] != data[k]) {
                    cout << k << endl;
                    output(buf, k, size);
                    output(data, k, size);
                    fflush(stdout);
                }
                assert(buf[k] == data[k]);
            }
            // cout << "testSuccess: " << i << endl;
        }
        cout << "IO Test success" << endl;
    }

    static void testMutex() {
        const int testCnt = 1024;
        const int maxMutexCnt = 1024;
        for (int i = 0; i < testCnt; i++) {
            const int len = static_cast<const int>(random() % maxMutexCnt);
            auto *mutex = new pthread_mutex_t[len];
            for (int j = 0; j < len; j++) {
                assert(mutexInit(mutex[j]));
            }
            if (len >= 1) {
                assert(mutexLock(mutex[0]));
                mutexUnlock(mutex[0]);
            }
            for (int k = 0; k < len; k++) {
                mutexDestroy(mutex[k]);
            }
        }
    }

    static void testCondition() {
        const int testCnt = 1024;
        const int maxCondCnt = 1024;
        for (int i = 0; i < testCnt; i++) {
            const int len = static_cast<const int>(random() % maxCondCnt);
            auto *cond = new pthread_cond_t[len];
            for (int j = 0; j < len; j++) {
                assert(conditionInit(cond[j]));
            }
            pthread_mutex_t mutex;
            assert(mutexInit(mutex));
            // if (len >= 1) {
            //     // TODO the wait doesn't timeout
            //     conditionWait(cond[0], mutex, 1);
            // }
            for (int k = 0; k < len; k++) {
                conditionDestroy(cond[k]);
            }
        }
    }

    static void testThread() {
        pthread_t pid;
        cout << createThread(pid, (UtilityTest::runner)) << endl;
    }

    static void testSignal() {
        changeThreadSigMask((const int[]) {SIGINT, 0}, SIG_BLOCK);
        kill(getpid(), SIGINT);
    }

    static void testNetwork(const char *const listenPort) {
        int listenFd = openListenFd(listenPort);
        assert(listenFd >= 3);
        string thisMachineIP = getThisMachineIp();
        int clientFd = openClientFd(thisMachineIP.c_str(), listenPort);
        assert(clientFd >= 3 && clientFd != listenFd);
        int ac1 = acceptConnect(listenFd);
        assert(ac1 >= 3 && ac1 != clientFd && ac1 != listenFd);
        string msg = "testMsg\r\n";
        writeAllData(clientFd, msg.c_str(), msg.size());
        char buf[100];
        ReadBuf cache;
        RecvLineReturnValue value = readLine(ac1, buf, 100 - 1, cache);
        assert(value.recvCnt == msg.size());
        assert(value.isEndOfLine);
        assert(!value.isEOF);
        assert(value.success);
        readLine(ac1, buf, 100 - 1, cache);
        assert(value.recvCnt == 0);
        assert(value.isEOF);
        assert(!value.isEndOfLine);
        assert(!value.success);
    }

    static void testNetwork() {

    }

private:

    static void *runner(void *argv) {
        cout << "subThread" << endl;
        return nullptr;
    }

    static void output(char *s, int begin, int end) {
        for (int i = begin; i < end; i++) {
            cout << (int) (s[i]) << " ";
        }
        cout << endl;
    }
};


#endif //FTP_SERVER_UTILITY_TEST_H
