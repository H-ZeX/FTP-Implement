- 打开一个clientFd，然后关闭（关闭前对端还没有调用accept），然后重新打开。对端accept得到的fd对应的链接不是client端第二次打开的链接
   ```cpp
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
   ```