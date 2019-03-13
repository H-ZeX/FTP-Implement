# FTP-Implement

### Refactor

- This project is refactoring, while refactoring, it can not be built.

### Overview

- Implement according to RFC 959
- Use thread pool, `epoll` to accept new user and uses' command
- Does NOT support the authority control. Because that I use threadPool but the posix's setuid and `syscall(SYS_setuid, uid)` don't support setting other thread's uid(If you know how to do it, please tell me. Thank you!).
- **Should use `sudo` to run this ftp server, for that it use operating system's accout login mechanism.**
- Develop and test on linux kernel >= 4.10 (debian series), so I don't know whethere it will work fine at other kernel or operating system.

