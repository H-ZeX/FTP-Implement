//
// Created by hzx on 19-3-15.
//

#ifndef FTP_SERVER_CORE_TEST_HPP
#define FTP_SERVER_CORE_TEST_HPP

#include <src/main/core/Login.hpp>
#include "src/main/core/NetworkManager.hpp"
#include "src/main/core/Session.hpp"

class CoreTest {
public:
    static void testLogin() {
        UserInfo me = login("hzx", "nhzsmjrsgdl", "", "");
        assert(me.isValid);
        cout << me.uid << " " << me.gid << " " << me.homeDir << endl;
        me = login("hzx", "ss", "", "");
        assert(!me.isValid);
        me = login("hz", "ed", "", "");
        assert(!me.isValid);
    }

    static void testNetworkSession() {
        const int testCnt = 1024;
        for (int j = 0; j < testCnt; j++) {
            NetworkManager session;
            int cmdListenFd = openListenFd("8080");
            assert(cmdListenFd >= 3);
            int cmdClientFd = openClientFd("localhost", "8080");
            assert(cmdClientFd >= 3);
            int cmdServerFd = acceptConnect(cmdListenFd);
            assert(cmdServerFd >= 3);
            session.setCmdFd(cmdServerFd);

            string msg = "testMsg\r\ntestMsgTestMsg\r\n";
            char buf[1024];

            {
                assert(writeAllData(cmdClientFd, msg.c_str(), msg.length()));
                assert(closeFileDescriptor(cmdClientFd));
                RecvCmdReturnValue ret = session.recvCmd(buf, sizeof(buf));
                assert(ret.success);
                assert(!ret.isEOF);
                assert(ret.cnt == 7);
                assert(string(buf)=="testMsg");
                ret = session.recvCmd(buf, 1024);
                assert(ret.success);
                assert(ret.success);
                assert(!ret.isEOF);
                assert(ret.cnt == 14);
                assert(string(buf)=="testMsgTestMsg");
                ret = session.recvCmd(buf, 1024);
                assert(!ret.success);
                assert(!ret.success);
                assert(ret.isEOF);
                closeFileDescriptor(cmdServerFd);
            }
            {
                OpenDataListenReturnValue ret2 = session.openDataListen();
                assert(ret2.success);
                int clientDataFd = openClientFd("localhost", to_string(ret2.port).c_str());
                assert(clientDataFd >= 3);
                assert(session.acceptDataConnect());

                session.sendToDataFd(msg.c_str(), msg.length());
                assert(session.closeDataListen());
                assert(session.closeDataConnect());

                ReadBuf cache{};
                ReadLineReturnValue ret3 = readLine(clientDataFd, buf, sizeof(buf), cache);
                assert(ret3.success);
                assert(!ret3.isEOF);
                assert(ret3.recvCnt == 7);
                assert(string(buf)=="testMsg");
                ret3 = readLine(clientDataFd, buf, sizeof(buf), cache);
                assert(ret3.success);
                assert(ret3.success);
                assert(!ret3.isEOF);
                assert(ret3.recvCnt == 14);
                assert(string(buf)=="testMsgTestMsg");
                ret3 = readLine(clientDataFd, buf, 1024, cache);
                assert(!ret3.success);
                assert(!ret3.success);
                assert(ret3.isEOF);

                assert(closeFileDescriptor(clientDataFd));
            }
            {
                OpenListenFdReturnValue ret = openListenFd(1);
                assert(ret.success);
                assert(session.openDataConnection("localhost", to_string(ret.port).c_str()));
                int dataServerFd = acceptConnect(ret.listenFd);
                assert(dataServerFd >= 3);
                assert(writeAllData(dataServerFd, msg.c_str(), msg.length()));
                closeFileDescriptor(dataServerFd);
                session.recvRemoteAndWriteLocalFile("/tmp/f1");
                session.closeDataConnect();

                assert(session.openDataConnection("localhost", to_string(ret.port).c_str()));
                dataServerFd = acceptConnect(ret.listenFd);
                assert(dataServerFd >= 3);
                session.sendLocalFile("/tmp/f1");
                session.closeDataConnect();

                ReadBuf cache{};
                ReadLineReturnValue ret3 = readLine(dataServerFd, buf, sizeof(buf), cache);
                assert(ret3.success);
                assert(!ret3.isEOF);
                assert(ret3.recvCnt == 7);
                assert(string(buf)=="testMsg");
                ret3 = readLine(dataServerFd, buf, sizeof(buf), cache);
                assert(ret3.success);
                assert(ret3.success);
                assert(!ret3.isEOF);
                assert(ret3.recvCnt == 14);
                assert(string(buf)=="testMsgTestMsg");
                ret3 = readLine(dataServerFd, buf, 1024, cache);
                assert(!ret3.success);
                assert(!ret3.success);
                assert(ret3.isEOF);
                assert(closeFileDescriptor(dataServerFd));
                assert(closeFileDescriptor(ret.listenFd));
            }
            assert(closeFileDescriptor(cmdListenFd));
        }
    }

    // static void testParsePortCmd() {
    //     string ip;
    //     int port = 0;
    //     Session session(10, nullptr, nullptr, "");
    //     session.parsePortCmd("   127,0,0,1,1,1", ip, port);
    //     assert(ip == "127.0.0.1");
    //     assert(port == 256 * 1 + 1);
    //     session.parsePortCmd("127,0,0,1,1,1", ip, port);
    //     assert(ip == "127.0.0.1");
    //     assert(port == 256 * 1 + 1);
    //     session.parsePortCmd("127,  0 ,0,1 ,1,  1", ip, port);
    //     assert(ip == "127.0.0.1");
    //     assert(port == 256 * 1 + 1);
    //     session.parsePortCmd("127,  0   ,0,1 ,  1,  1", ip, port);
    //     assert(ip == "127.0.0.1");
    //     assert(port == 256 * 1 + 1);
    // }

    static void testSession() {
        const int testCnt = 32;
        for (int i = 0; i < testCnt; i++) {
            OpenListenFdReturnValue ret = openListenFd();
            assert(ret.success);
            int cmdClientFd = openClientFd("localhost", to_string(ret.port).c_str());
            assert(cmdClientFd >= 3);
            int cmdServerFd = acceptConnect(ret.listenFd);
            assert(cmdServerFd >= 3);

            Session session(cmdServerFd, nullptr, nullptr, getThisMachineIp());
            char clientCmdBuf[1024]{};
            const size_t clientCmdBufSize = sizeof(clientCmdBuf) - 10;
            ReadBuf clientCmdCache{};

            string loginReq = "user hzx\r\n";
            string loginResp = "331 Please specify the password";
            assert(writeAllData(cmdClientFd, loginReq.c_str(), loginReq.length()));
            session.handle();
            ReadLineReturnValue ret2 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
            assert(ret2.success);
            assert(ret2.isEndOfLine);
            assert(ret2.recvCnt == loginResp.length());
            assert(loginResp == string(clientCmdBuf));

            string passReq = "pass nhzsmjrsgdl\r\n";
            string passResp = "230 Login successful.";
            assert(writeAllData(cmdClientFd, passReq.c_str(), passReq.length()));
            session.handle();
            ReadLineReturnValue ret3 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
            assert(ret3.success);
            assert(ret3.isEndOfLine);
            assert(ret3.recvCnt == passResp.length());
            assert(passResp == string(clientCmdBuf));

            string cdReq = "cwd /tmp/tdir\r\n";
            string cdResp = "250 Directory successfully changed";
            assert(writeAllData(cmdClientFd, cdReq.c_str(), cdReq.length()));
            session.handle();
            ReadLineReturnValue ret9 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
            assert(ret9.success);
            assert(ret9.isEndOfLine);
            assert(ret9.recvCnt == cdResp.length());
            assert(!ret9.isEOF);
            assert(cdResp == string(clientCmdBuf));

            int dataClientFd = port(cmdClientFd, session, clientCmdCache);

            string listReq = "list /tmp/tdir\r\n";
            string listResp1 = "150 Here comes the directory listing.";
            string listResp2 = "226 Directory send OK.";
            assert(writeAllData(cmdClientFd, listReq.c_str(), listReq.length()));
            session.handle();
            ReadLineReturnValue ret4 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
            assert(ret4.success);
            assert(ret4.isEndOfLine);
            assert(ret4.recvCnt == listResp1.length());
            assert(!ret4.isEOF);
            assert(listResp1 == string(clientCmdBuf));

            char clientDataBuf[10240];
            const int clientDataBufSize = sizeof(clientDataBuf) - 10;
            ReadBuf clientDataCache{};
            int dataLen = 0;
            {
                char *ptr = clientDataBuf;
                char *end = clientDataBuf + clientDataBufSize;
                int t = readWithBuf(dataClientFd, clientDataBuf, clientDataBufSize, clientDataCache);
                while (t > 0 && ptr < end) {
                    ptr += t;
                    dataLen += t;
                    t = readWithBuf(dataClientFd, ptr, static_cast<int>(end - ptr), clientDataCache);
                }
            }
            closeFileDescriptor(dataClientFd);
            assert(dataLen > 0);
            clientDataBuf[dataLen] = 0;
            string lsData;
            FileSystem::ls("/tmp/tdir", lsData);
            assert(string(clientDataBuf)== lsData);

            ReadLineReturnValue ret6 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
            assert(!ret6.isEOF);
            assert(ret6.isEndOfLine);
            assert(ret6.recvCnt == listResp2.length());
            assert(ret6.success);

            string pwdReq = "pwd\r\n";
            string pwdResp = "257 \"/tmp/tdir/\" is the current directory";
            assert(writeAllData(cmdClientFd, pwdReq.c_str(), pwdReq.length()));
            session.handle();
            ret4 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
            assert(ret4.success);
            assert(ret4.isEndOfLine);
            assert(ret4.recvCnt == pwdResp.length());
            assert(!ret4.isEOF);
            assert(pwdResp == string(clientCmdBuf));

            int clientDataFd = port(cmdClientFd, session, clientCmdCache);
            string retrReq = "retr /tmp/udaa\r\n";
            string retrResp = "150 Opening data connection for /tmp/udaa";
            string retrResp2 = "226 Transfer complete.";
            assert(writeAllData(cmdClientFd, retrReq.c_str(), retrReq.length()));
            session.handle();
            ReadLineReturnValue ret8 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
            assert(ret8.success);
            assert(ret8.recvCnt == retrResp.length());
            assert(ret8.isEndOfLine);
            assert(!ret8.isEOF);
            assert(string(clientCmdBuf)==retrResp);

            vector<char> data;
            vector<char> data2;
            {
                ReadBuf cache{};
                assert(readAllData(data, clientDataFd, cache));
                closeFileDescriptor(clientDataFd);
            }
            {
                int fd = openWrapV1("/tmp/udaa", O_RDONLY);
                assert(fd >= 3);
                ReadBuf cache{};
                assert(readAllData(data2, fd, cache));
                closeFileDescriptor(fd);
            }
            assert(data.size() == data2.size());
            for (size_t j = 0; j < data.size(); j++) {
                assert(data[j] == data2[j]);
            }
            ret8 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
            assert(ret8.success);
            assert(ret8.recvCnt == retrResp2.length());
            assert(ret8.isEndOfLine);
            assert(!ret8.isEOF);
            assert(string(clientCmdBuf)==retrResp2);

            assert(closeFileDescriptor(ret.listenFd));
            assert(closeFileDescriptor(cmdServerFd));
            assert(closeFileDescriptor(cmdClientFd));
        }
    }

    static int port(int cmdClientFd, Session &session, ReadBuf &clientCmdCache) {
        const int clientCmdBufSize = 1024;
        char clientCmdBuf[clientCmdBufSize]{};

        OpenListenFdReturnValue ret5 = openListenFd(1);
        assert(ret5.success);
        string pasvReq = "port 127,0,0,1, "
                         + to_string(ret5.port / 256) + ", "
                         + to_string(ret5.port % 256)
                         + " \r\n";
        string pasvResp = "200 PORT command successful. Consider using PASV.";
        assert(writeAllData(cmdClientFd, pasvReq.c_str(), pasvReq.length()));
        session.handle();
        ReadLineReturnValue ret4 = readLine(cmdClientFd, clientCmdBuf, clientCmdBufSize, clientCmdCache);
        assert(ret4.success);
        assert(ret4.isEndOfLine);
        assert(ret4.recvCnt == pasvResp.length());
        assert(!ret4.isEOF);
        assert(pasvResp == string(clientCmdBuf));

        int dataClientFd = acceptConnect(ret5.listenFd);
        closeFileDescriptor(ret5.listenFd);
        assert(dataClientFd >= 3);
        return dataClientFd;
    }

    static void testSpecialConnect() {
        OpenListenFdReturnValue ret = openListenFd(1);
        assert(ret.success);
        int clientFd = openClientFd("localhost", to_string(ret.port).c_str());
        assert(clientFd >= 3);
        // closeFileDescriptor(clientFd);
        // clientFd = openClientFd("localhost", to_string(ret.port).c_str());
        // assert(clientFd >= 3);
        int serverFd = acceptConnect(ret.listenFd);
        string msg = "testMsg\r\n";
        assert(writeAllData(serverFd, msg.c_str(), msg.length()));
        closeFileDescriptor(serverFd);
        char clientBuf[1024]{};
        const int clientBufSize = sizeof(clientBuf) - 10;
        ReadBuf clientCache{};
        ReadLineReturnValue ret2 = readLine(clientFd, clientBuf, clientBufSize, clientCache);
        printf("%s\n", clientBuf);
        assert(ret2.success);
        assert(ret2.recvCnt == msg.length() - 2);
    }
};

#endif //FTP_SERVER_CORE_TEST_HPP
