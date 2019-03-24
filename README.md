# FTP-Implement

- [中文的介绍](https://h-zex.github.io/2019/03/20/%E5%BC%80%E5%8F%91%E4%B8%80%E4%B8%AA%E9%AB%98%E5%B9%B6%E5%8F%91%E7%9A%84FTP%E6%9C%8D%E5%8A%A1%E5%99%A8/)

### Overview

- Implement according to RFC 959
- C2K
- Should use `sudo` to run this FTP server, for that it use operating system's accout login mechanism.
- Develop and test on linux kernel >= 4.10 (debian series), so I don't know whethere it will work fine at other kernel or operating system.
- Does NOT support the authority control. Because that I use threadPool but the posix's setuid and `syscall(SYS_setuid, uid)` don't support setting other thread's uid(If you know how to do it, please tell me. Thank you!).


### StressTest


#### Step

- First, run the FTP server
  ```
  ulimit -s unlimited -f unlimited -d unlimited -n unlimited 
  su root
  echo 20000 >  /proc/sys/net/core/somaxconn
  sudo ./FTPServer [port]  > /tmp/FTPServerOutput
  ```
  
  Redirect the stdout to `/tmp/FTPServerOutput` is for the clear output of warning msg

  Change `somaxconn` is to make the backlog large enough.

  The `sudo` is because that it use operating system's accout login mechanism

  The `[port]` param of `./FTPServer [port]` is not necessary, which means `sudo ./FTPServer  > /tmp/FTPServerOutput`, this will **listen on `8001`**

- Then run `java  -ea -jar -Dexternal.config=file:/tmp/1.properties FTPServerTester-1.0-SNAPSHOT.jar`

  `FTPServerTester-1.0-SNAPSHOT.jar` is in the `test` dir （NOT `src/test`）

  `/tmp/1.properties` is config file, the example is below

  ```properties
  StressTest.TestCnt=10
  StressTest.MaxCmdConnectionCnt=1000
  StressTest.MaxThreadCnt=1024
  # the time(millisecond) to hand on the connection
  StressTest.HangTime=1000
  
  # the host that you run the ftp server
  Tester.TesterServerAddress=10.243.6.109
  
  # the host that you run the tester
  # MUST make sure that this two host can connect to each other,
  # which means, they can active connection to each other.
  Tester.YourselfAddress=10.243.6.43


  Tester.ServerPort=8001
  # The username of account in your OS
  Tester.UserName=
  # The password of account in your OS
  Tester.Password=
  # These dir MUST be absoute path
  # this is the list of dirs that split by comma
  # This dir's cnt should >=20, the more, the better, 
  # if too less, the test will be very slow, for that many client read the same dir is very slow
  # These dir should exist in your testServer, if the server and testProgram run on different host
  # if they run in the same host, then the testProgram will create these dir
  Tester.ListTestDir=/tmp/testDir_1,/tmp/testDir_2,/tmp/testDir_3,/tmp/testDir_4,/tmp/testDir_5,/tmp/testDir_6,/tmp/testDir_7,/tmp/testDir_8,/tmp/testDir_9,/tmp/testDir_10,/tmp/testDir_11,/tmp/testDir_12,/tmp/testDir_13,/tmp/testDir_14,/tmp/testDir_15,/tmp/testDir_16,/tmp/testDir_17
  # this is one dir 
  # This MUST be absoute path 
  # This dir should exist in your testServer, if the server and testProgram run on different host
  # if they run in the same host, then the testProgram will create this dir
  Tester.StorTestDir=/tmp/FTPSeverTesterStorDirs____23233dd22/
  ```

#### Result

- In my computer(Intel i7-8550U, 16G memory, no SSD), when the `StressTest.MaxCmdConnectionCnt` below **`10240`**, the test will run success, and using linux's `ftp` command to communicate with the server while run the test, it response fast.

