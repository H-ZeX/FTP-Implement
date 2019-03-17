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

- 测试时发现，有些用户打开socket成功，但是总是收不到welcome信息，并且服务器的log显示根本就没有accept到这些用户（accept的数量少于请求的用户数量，并且少的数量刚好与等待welcome信息的用户数量相同）。一开始怀疑是epoll的问题，后来发现，把backlog开大即可解决问题。我的理解如下：backlog是完成三次握手的queue的大小，如果服务端很忙，可能该queue满了，一些fd被抛弃，这些fd完成了三次握手，但是抛弃时没收到fin或rst，结果就hang住了。
