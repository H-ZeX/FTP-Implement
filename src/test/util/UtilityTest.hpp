//
// Created by hzx on 19-3-8.
//

#ifndef FTP_SERVER_UTILITY_TEST_H
#define FTP_SERVER_UTILITY_TEST_H

#include <cassert>
#include "src/main/util/Utility.hpp"
#include "src/main/util/ThreadUtility.hpp"
#include "src/main/util/NetUtility.hpp"


class UtilityTest {
public:

    static void testBug() {
        bug("TestBug");
    }

    static void testBugWithErrno() {
        bugWithErrno("testBugWithErrno", errno, true);
    }

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

    // TODO mutexInit's error situation had NOT tested
    static void testMutex(const int testCnt = 1024) {
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
            delete[](mutex);
        }
    }

    static void testTimedMutex() {
        pthread_mutex_t mutex;
        assert(mutexInit(mutex));
        assert(mutexLock(mutex));
        assert(!mutexLock(mutex, 10));
    }

    // TODO condInit's error situation had NOT tested
    static void testCondition(const int testCnt = 1024) {
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
            delete[] (cond);
        }
    }

    static void testConditionSignal() {
        pthread_cond_t cond1{};
        conditionInit(cond1);
        conditionSignal(cond1);

        pthread_cond_t cond2;
        conditionSignal(cond2);
    }

    static void testConditionBroadcast() {
        pthread_cond_t cond;
        conditionInit(cond);
        conditionBroadcast(cond);

        pthread_cond_t cond2;
        conditionBroadcast(cond2);
    }


    // TODO valgrind report a leak here
    static void testThread() {
        pthread_t pid;
        cout << createThread(pid, (UtilityTest::runner)) << endl;
    }

    static void testSignal() {
        changeThreadSigMask((const int[]) {SIGINT, 0}, SIG_BLOCK);
        kill(getpid(), SIGINT);
    }

    static void testNetworkAndReadLine(const char *const listenPort, const int testCnt = 1024) {
        const int maxBufSize = 1024;
        const int maxMsgLen = 1024 * 10;
        const int maxLineLen = 100;

        const int listenFd = openListenFd(listenPort);
        assert(listenFd >= 3);
        const string thisMachineIP = getThisMachineIp();

        // TODO, only test when the buf can contain the full line.
        // should add test that the buf can not contain one line.
        for (int i = 0; i < testCnt; i++) {
            const int bufSize = static_cast<int>(random() % maxBufSize + maxLineLen);
            const int msgLen = static_cast<int>(random() % maxMsgLen);
            const string msg = renderString(msgLen, maxLineLen);

            vector<int> spIndex;
            spIndex.push_back(static_cast<int &&>(msg.find("\r\n", 0)));
            if (*spIndex.rbegin() != string::npos) {
                int index = *spIndex.rbegin() + 2;
                while (index >= 0 && index < msg.size()) {
                    int t = static_cast<int &&>(msg.find("\r\n", index));
                    if (t == string::npos) {
                        break;
                    }
                    spIndex.push_back(t);
                    index = *spIndex.rbegin() + 2;
                }
            } else {
                spIndex.clear();
            };

            const int clientFd = openClientFd(thisMachineIP.c_str(), listenPort);
            assert(clientFd >= 3 && clientFd != listenFd);
            const int ac1 = acceptConnect(listenFd);
            assert(ac1 >= 3 && ac1 != clientFd && ac1 != listenFd);

            assert(writeAllData(clientFd, msg.c_str(), msg.size()));
            assert(closeFileDescriptor(clientFd));

            char buf[bufSize];
            ReadBuf cache;
            int before = 0;
            for (int j : spIndex) {
                ReadLineReturnValue value = readLine(ac1, buf, static_cast<unsigned long>(bufSize - 1), cache);
                if ((value.recvCnt != (j - before))) {
                    for (int p : spIndex) {
                        cerr << p << "\t";
                    }
                    cerr << endl;
                    outputStr(msg);
                    cerr << (value.recvCnt) << "\t"
                         << (j - before) << "\t"
                         << j << "\t"
                         << before << "\t"
                         << endl
                         << buf << endl
                         << bufSize << endl;
                }
                assert(value.recvCnt == (j - before));
                // if EndOfLine is true, EOF is always false;
                assert(!value.isEOF);
                assert(value.isEndOfLine);
                assert(value.success);
                before = j + 2;
            }
            ReadLineReturnValue value = readLine(ac1, buf, static_cast<unsigned long>(bufSize - 1), cache);
            assert(!value.success);
            assert(!value.isEndOfLine);
            assert(value.isEOF);
            assert(value.recvCnt == (msg.size() - before));
        }
    }

    static void testConsumeUntilEndOfLine() {
        int fd[2];
        pipe(fd);
        string msg = "testMsg\r\ntestMsgTestMsg\r\n";
        write(fd[1], msg.c_str(), msg.size());
        ReadBuf cache;
        assert(!consumeByteUntilEndOfLine(fd[0], cache));
        char buf[100];
        readLine(fd[0], buf, 100 - 1, cache);
        assert(string(buf)=="testMsgTestMsg");
    }

    static void testGetUidGidHomeDir() {
        UserInfo info = getUidGidHomeDir("hzx");
        cout << info.cmdIp << endl
             << info.cmdPort << endl
             << info.gid << endl
             << info.homeDir << endl
             << info.isValid << endl
             << info.uid << endl
             << info.username << endl;
    }


private:

    static void outputStr(string s) {
        for (char i : s) {
            if (i == '\r') {
                cerr << "\\" << "r";
            } else if (i == '\n') {
                cerr << "\\" << "n";
            } else {
                cerr << i;
            }
        }
        cerr << endl;
    }

    static string renderString(int len, int maxLineLen) {
        string result;
        int lineLen = 0;
        for (int i = 0; i < len; i++) {
            int a = static_cast<int>(random() % 5);
            if (a == 0 || maxLineLen - lineLen < 5) {
                result += "\r\n";
                lineLen = 0;
            } else if (a == 1) {
                result += "\n";
                lineLen += 1;
            } else if (a == 2) {
                result += "\r";
                lineLen += 1;
            } else {
                // should render less than 4 char, or the lineLen judge will fail
                string s = std::to_string(random() % 1000);
                assert(s.size() <= 3);
                result += s;
                lineLen += s.size();
            }
        }
        return result;
    }

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
