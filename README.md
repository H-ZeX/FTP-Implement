# FTP-Implement

### Overview

- Implement according to RFC 959
- Use thread pool, `epoll` to accept new user and uses' command
- Does NOT support the authority control. For that the posix's setuid change all process' uid
   > However,  POSIX  requires  that  all threads in a process share the same credentials.  The NPTL threading implementation handles the  POSIX requirements by providing wrapper functions for the various system calls that change process UIDs and GIDs.  These  wrapper  func‐ tions (including the one for setuid()) employ a signal-based technique to ensure that when one thread changes credentials, all of  the  other threads  in  the  process also change their credentials.  For details, see nptl(7).(ref from setuid's manual)

   and that, although `syscall(SYS_setuid, uid)` work for thread itself, what I use is threadPool, this make that I can't use this way to change uid. So the management of authorization is not implemented.

- **Should use `sudo` to run this ftp server, for that it use operating system's accout login mechanism.**
- Develop and test on linux kernel >= 4.10 (debian series), so I don't know whethere it will work fine at other kernel or operating system

### Test

- More test is need to add

