//
// Created by hzx on 19-3-15.
//

#ifndef FTP_SERVER_CORE_TEST_HPP
#define FTP_SERVER_CORE_TEST_HPP

#include <src/main/core/Login.hpp>
#include "src/main/core/NetworkSession.hpp"

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
            NetworkSession session;
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
                closeFileDescriptor(cmdListenFd);
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

                ReadBuf cache;
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
            }
        }
    }
};

#endif //FTP_SERVER_CORE_TEST_HPP
