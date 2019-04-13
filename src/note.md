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
   - set logging on
   - set logging file my_god_object.log
   - 调试时的signal函数设置的信号handler似乎无用，比如我设置了SIGPIPE的handler，但是gdb还是反馈收到SIGPIPE，而不用gdb则是调用我设置的信号handler

- 往对端已经关闭的socket写数据，会返回RST，但是write会返回写了>0字节
   ```c
   static void test4() {
       OpenListenFdReturnValue ret = openListenFd(1);
       assert(ret.success);

       int client = openClientFd("localhost", to_string(ret.port).c_str());
       assert(client >= 3);
       int server = acceptConnect(ret.listenFd);
       closeFileDescriptor(client);
       sleep(3);
       assert(write(server, "s", 1) == 1);
   }
   ```
- 出现一种情况，测试写10MB的文件，然后在测到中途时关闭，服务端会有一些establish链接，netstat发现，client端是`FIN_WAIT1`，wireshare没有抓到FIN包。测试参数如下
  ```properties
  StressTest.TestCnt=10
  StressTest.MaxCmdConnectionCnt=10000
  StressTest.MaxThreadCnt=4024
  # the time(millisecond) to hand on the connection
  StressTest.HangTime=0
  Tester.TesterServerAddress=127.0.0.1
  Tester.YourselfAddress=127.0.0.1
  Tester.ServerPort=8001
  Tester.UserName=hzx
  Tester.Password=...
  Tester.ListTestDir=/tmp/3,/tmp/4,/tmp/5,/tmp/6,/tmp/7,/tmp/8,/tmp/9,/tmp/10,/tmp/11
  Tester.StorTestDir=/tmp/seed/
  ```
- brokenPipe：如果某个write返回rst，再次write就会broken pipe，broken pipe的含义是往read端关闭的管道写东西

- java中对于线程池的shutdown一定要在finally子句完成，不然很可能因为RuntimeException而没机会shutdown，结果整个线程池hang在那里

- 对map.find返回的iterator的修改是可以反映在map上的，说明其实有树内的引用的。如果对map的某个元素erase，然后又使用find返回的iterator，就会出问题
