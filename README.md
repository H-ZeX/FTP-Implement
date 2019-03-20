# FTP-Implement

- [中文的介绍](https://h-zex.github.io/2019/03/20/%E5%BC%80%E5%8F%91%E4%B8%80%E4%B8%AA%E9%AB%98%E5%B9%B6%E5%8F%91%E7%9A%84FTP%E6%9C%8D%E5%8A%A1%E5%99%A8/)

### Overview

- Implement according to RFC 959
- C10K
- Does NOT support the authority control. Because that I use threadPool but the posix's setuid and `syscall(SYS_setuid, uid)` don't support setting other thread's uid(If you know how to do it, please tell me. Thank you!).
- **Should use `sudo` to run this ftp server, for that it use operating system's accout login mechanism.**
- Develop and test on linux kernel >= 4.10 (debian series), so I don't know whethere it will work fine at other kernel or operating system.


### StressTest


#### Step

- First, run the FTP server
  ```
  ulimit -s unlimited -f unlimited -d unlimited -n unlimited 
  sudo ./FTPServer [port]  > /tmp/FTPServerOutput
  ```
  
  Redirect the stdout to `/tmp/FTPServerOutput` is for the clear output of warning msg

  The `sudo` is because that it use operating system's accout login mechanism, if no `sudo`, the problem will report a bug and exit

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
  Tester.TesterServerAddress=127.0.0.1
  Tester.ServerPort=8001
  Tester.UserName=
  Tester.Password=
  # These dir MUST NOT exist, MUST be absoute path
  # this is the list of dirs that split by comma
  Tester.ListTestDir=/tmp/testDir_1,/tmp/testDir_2,/tmp/testDir_3,/tmp/testDir_4,/tmp/testDir_5,/tmp/testDir_6,/tmp/testDir_7,/tmp/testDir_8,/tmp/testDir_9,/tmp/testDir_10,/tmp/testDir_11,/tmp/testDir_12,/tmp/testDir_13,/tmp/testDir_14,/tmp/testDir_15,/tmp/testDir_16,/tmp/testDir_17
  # this is one dir 
  # This dir MUST NOT exist, MUST be absoute path 
  Tester.StorTestDir=/tmp/FTPSeverTesterStorDirs____23233dd22/
  ```

#### Result

- In my computer(Intel i7-8550U, 16G memory, no SSD), when the `StressTest.MaxCmdConnectionCnt` below **`10240`**, the test will run success, and using linux's `ftp` command to communicate with the server is very smooth
