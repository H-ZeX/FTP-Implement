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
   `shutdown(SHUT_WR)`也是如此。原因在于，调用close、shutdown(SHUT_WR)之后，就进入半关闭状态，只能接受不能发送。所以这里对端调用write没有问题（而之所以read不到，是因为使用的是close而不是shutdown(SHUT_WR)）。如果对端调用的是read，则对端会得到一个错误。`shutdown(SHUT_RD)`似乎没作用，也没发fin报文，也可以正常读

- gdb调试
   - thread apply all bt
   - info threads 可以看到所有线程当前在哪
