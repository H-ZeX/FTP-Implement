#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "src/main/config/config.hpp"

#include "Def.hpp"

#include <arpa/inet.h>
#include <dirent.h>
#include <execinfo.h>
#include <fcntl.h>
#include <grp.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <cassert>
#include <sys/time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <sys/epoll.h>

using namespace std;

/*
 * Many errno is a legal error,
 * It is hard to find all bug according to the errno.
 * So the bug report is NOT complete.
 */

/**
 * MT-Safety
 */
void myLog(const char *const msg) {
    printf("%s\n", msg);
    fflush(stdout);
}

/**
 * MT-Safe locale
 */
void bug(const char *const msg, bool willExit = true, int lineNum = -1) {
    void *buffer[BT_BUF_SIZE];
    int t = backtrace(buffer, BT_BUF_SIZE);
    char **str = backtrace_symbols(buffer, t);
    if (str == nullptr) {
        fprintf(stderr, "backtrace_symbols error");
    }
    flockfile(stderr);
    fprintf(stderr, "%s\nLineNumber: %d\n", msg, lineNum);
    if (str != nullptr) {
        for (int i = 0; i < t; i++) {
            fprintf(stderr, "%s\n", str[i]);
        }
    }
    funlockfile(stderr);
    // delete nullptr is legal
    delete str;
    if (willExit) {
        exit(BUG_EXIT);
    }
}

/**
 * MT-Safe locale
 */
void bugWithErrno(const char *const msg, errno_t code, bool willExit) {
    char errnoBuf[ERRNO_BUF_SIZE], warningBuf[WARNING_BUF_SIZE];
    if (snprintf(warningBuf, WARNING_BUF_SIZE, "%s, errMsg: %s", msg,
                 strerror_r(code, errnoBuf, ERRNO_BUF_SIZE)) >= WARNING_BUF_SIZE) {
        bug("bugWithErrno snprintf buffer size too small", true);
    }
    bug(warningBuf, willExit);
}

/**
 * MT-Safe locale
 */
void warning(const char *const msg) {
    fprintf(stderr, "%s\n", msg);
}

/**
 * MT-Safe locale
 */
void warningWithErrno(const char *const msg, errno_t code) {
    char errnoBuf[ERRNO_BUF_SIZE], warningBuf[WARNING_BUF_SIZE];
    if (snprintf(warningBuf, WARNING_BUF_SIZE, "%s: %s", msg,
                 strerror_r(code, errnoBuf, ERRNO_BUF_SIZE)) >= WARNING_BUF_SIZE) {
        bug("warningWithErrno snprintf buffer size too small", true);
    }
    warning(warningBuf);
}

/**
 * @return -1 If  name corresponds to a maximum or minimum limit,
 * and that limit is indeterminate.
 * Or If name corresponds to an option and the option is not supported.
 * Otherwise, return the result
 *
 * MT-Safe env locale
 */
long Sysconf(int name) {
    errno_t es = errno;
    errno = 0;
    long t = sysconf(name);
    if (errno) {
        bugWithErrno("Sysconf failed", errno, true);
    }
    errno = es;
    return t;
}

/**
 * @return the limit on the number of thread
 * for the real user ID of the calling process.
 *
 * MT-Safe locale
 */
size_t maxThreadCnt() {
    rlimit rl{};
    if (getrlimit(RLIMIT_NPROC, &rl) < 0) {
        bugWithErrno("getrlimit failed", errno, true);
        return DEFAULT_THREAD_CNT;
    } else {
        return std::min<rlim_t>(rl.rlim_cur, rl.rlim_max);
    }
}

/**
 * if the fd is not given type, this function will report an bug
 * @return if there is error occur, return false;
 *
 * Thread-Safety: Unknown
 */
bool makeSureFdType(int fd, int type) {
    int t = isfdtype(fd, type);
    if (t == 0) {
        bug("makeSureFdType failed");
    } else if (t < 0) {
        // May because of many IO error or bug, etc.
        // So just print an warning.
        warningWithErrno("makeSureFdType failed", errno);
    }
    return t == 1;
}


/**
 * @param maxLen the dest MUST has >= (maxLen+1) size
 *
 * MT-Safe
 */
char *mereString(char *dest, const char *const strArray[], size_t maxLen) {
    string result;
    for (int i = 0; strArray[i] != nullptr; i++) {
        result += strArray[i];
    }
    size_t len = min(maxLen, result.size());
    result.substr(0, len);
    for (int j = 0; j < len; j++) {
        dest[j] = result[j];
    }
    dest[len] = 0;
    return dest;
}

/**
 * calculate the length of num
 *
 * MT-Safe
 */
template<typename T>
int numLen(T num, int base) {
    int ans = 0;
    while (num) {
        num /= base;
        ans++;
    }
    return ans;
}

/**
 * MT-Safe
 */
char charUpper(char c) {
    return (char) ((c >= 'a' && c <= +'z') * (-0x20) + c);
}

/**
 * @return the dest point (same as the dest param)
 *
 * will NOT pad \0 at the end of dest.
 * <br/>
 * MT-Safe
 */
byte *byteCpy(byte *dest, const byte *const src, int n) {
    for (int i = 0; i < n; i++) {
        dest[i] = src[i];
    }
    return dest;
}

/**
 *
 * @param flags same as open function's flags param
 * @return -1 if failed, otherwise the fd
 */
int openWrapV1(const char *const path, int flags) {
    int fd = open(path, flags);
    while (fd < 0) {
        if (errno == EFAULT || errno == EINVAL) {
            bugWithErrno("openWrap open failed", errno, true);
        } else if (errno == EINTR) {
            fd = open(path, flags);
            continue;
        } else {
            warningWithErrno("openWrap open failed", errno);
            return -1;
        }
    }
    return fd;
}

/**
 * @param flag1 same as open function's first flags param
 * @param flag2 same as open function's second flags param
 * @return -1 if failed, otherwise the fd
 */
int openWrapV2(const char *const path, int flag1, int flag2) {
    int fd = open(path, flag1, flag2);
    while (fd < 0) {
        if (errno == EFAULT || errno == EINVAL) {
            bugWithErrno("openWrap open failed", errno, true);
        } else if (errno == EINTR) {
            fd = open(path, flag1, flag2);
            continue;
        } else {
            warningWithErrno("openWrap open failed", errno);
            return -1;
        }
    }
    return fd;
}


struct ReadBuf {
    const size_t size = READ_BUF_SIZE;
    byte buf[READ_BUF_SIZE]{};
    // point to the start of haven't read substr
    byte *next = buf;
    //  remainderSize is size of byte that can be read
    int remainderSize = 0;
};

/**
 * @return The size had read, 0 if EOF(or want==0),
 * -1 if error occur, the errno will be set appropriately.
 * errno can be EAGAIN EWOULDBLOCK EBADF EINVAL EIO
 *
 * Thread-Safety: Unknown(because the `read` function's Thread-Safety is Unknown)
 */
int readWithBuf(int fd, byte *result, int want, ReadBuf &buf) {
    if (buf.remainderSize > 0) {
        int t = std::min<int>(buf.remainderSize, want);
        byteCpy(result, buf.next, t);
        buf.remainderSize -= t;
        if (buf.remainderSize == 0) {
            buf.next = buf.buf;
        } else {
            buf.next += t;
        }
        return t;
    }
    assert(buf.remainderSize == 0);
    do {
        ssize_t cnt = read(fd, buf.buf, buf.size);
        if (cnt < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            } else if (errno == EFAULT || errno == EISDIR) {
                bugWithErrno("readWithBuf read() failed", errno, true);
            } else if (errno == EINTR) {
                continue;
            } else {
                // may because of bug, close of fd, another peer close connection, etc,
                // so it is hard to handle here.
                // and report an warning msg is not very good.
                return -1;
            }
        } else if (cnt == 0) {
            return 0;
        } else if (cnt > 0) {
            buf.remainderSize += cnt;
            int t = std::min<int>(buf.remainderSize, want);
            byteCpy(result, buf.next, t);
            buf.remainderSize -= t;
            if (buf.remainderSize == 0) {
                buf.next = buf.buf;
            } else {
                buf.next += t;
            }
            return t;
        }
    } while (errno == EINTR);
    assert(false);
}

/**
 * @return return false if error occur.
 */
bool readAllData(vector<char> &result, int fd, ReadBuf &buf) {
    char readBuf[2048];
    const int readBufSize = sizeof(readBuf) - 10;
    int hadRead = 0;
    while (true) {
        hadRead = readWithBuf(fd, readBuf, readBufSize, buf);
        if (hadRead <= 0) {
            break;
        }
        for (int i = 0; i < hadRead; i++) {
            result.push_back(readBuf[i]);
        }
    };
    return hadRead == 0;
}

/**
 * Thread-Safety: Unknown(because the write function's Thread-Safety is Unknown
 */
bool writeAllData(int fd, const byte *buf, size_t size) {
    if (size == 0) {
        return true;
    }
    while (size > 0) {
        ssize_t r = write(fd, buf, size);
        if (r < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            } else if (errno == EDESTADDRREQ || errno == EFAULT || errno == EPERM) {
                bugWithErrno("writeAllData write failed", errno, true);
            } else {
                warningWithErrno("writeAllData write failed", errno);
                return false;
            }
        } else if (r == 0) {
            return false;
        } else {
            /*
             * Ref from write's manual.
             * If a write() is interrupted by a signal handler before any  bytes  are
             * written,  then  the  call  fails with the error EINTR; if it is interrupted
             * after at least one byte has been written,  the  call  succeeds,
             * and returns the number of bytes written.
             */
            size -= r;
            buf += r;
        }
    }
    return true;
}

/**
 * Thread-Safety: Unknown(Because the `close` function's Thread-Safety is Unknown)
 */
void closeFileDescriptor(int fd) {
    if (fd < 3 && fd >= 0) {
        bug(("closeFileDescriptor: the fd pass to me is illegal. fd: " + to_string(fd)).c_str());
    }
    do {
        int r = close(fd);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                bugWithErrno("closeFileDescriptor close failed", errno, true);
            }
        } else {
            return;
        }
    } while (errno == EINTR);
}

/**
 * Thread-Safety: Unknown(because of `fcntl` function's Thread-Safety is Unknown)
 */
bool setNonBlocking(int fd) {
    // TODO, the errno handler should be refined
    return (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) == -1);
}

struct UserInfo {
    bool isValid = false;
    uid_t uid = 0;
    gid_t gid = 0;
    string cmdIp = "";
    string cmdPort = "";
    string username = "";
    string homeDir = "";

    UserInfo() = default;

    UserInfo(bool isValid, uid_t uid, gid_t gid,
             const string &ip, const string &port,
             const string &username, const char *const homeDir) {
        this->isValid = isValid;
        this->uid = uid;
        this->gid = gid;
        this->cmdIp = ip;
        this->cmdPort = port;
        this->username = username;
        this->homeDir = homeDir;
    }
};

/**
 * Thread-Safety: MT-Safe locale
 */
UserInfo getUidGidHomeDir(const char *const username) {
    struct passwd pwd{}, *res = nullptr;
    long limit = Sysconf(_SC_GETPW_R_SIZE_MAX);
    limit = limit == -1 ? UNDETERMINED_LIMIT : limit;
    char buf[limit];
    int q;
    do {
        q = getpwnam_r(username, &pwd, buf, static_cast<size_t>(limit), &res);
    } while (q == EINTR);
    if (q != 0) {
        warningWithErrno("getUidGidHomeDir failed", q);
    }
    return UserInfo(q == 0 && res != nullptr, pwd.pw_uid, pwd.pw_gid, "", "", username, pwd.pw_dir);
}

struct ReadLineReturnValue {
    /**
     * whether successfully read a line.
     */
    bool success = false;
    bool isEndOfLine = false;
    bool isEOF = false;
    /**
     * this time's recv cnt, not include CRLF (if has CRLF)
     */
    size_t recvCnt = 0;

    ReadLineReturnValue(bool success, bool isEndOfLine, bool isEOF, size_t recvCnt)
            : success(success),
              isEndOfLine(isEndOfLine),
              isEOF(isEOF),
              recvCnt(recvCnt) {}

    explicit ReadLineReturnValue(bool success)
            : success(success) {}
};


/**
 * @note
 * if EndOfLine is true, then EOF will always be false
 *
 * @param size the buf len should >= size+1, after read, the buf[size] is 0.
 * if the size is too small, then may not contain one full line.
 * @param buf store the result, the index after the line's last char is 0.
 * the buf does NOT contain the CRLF
 *
 * MT-safety: Unknown(because of `read` function's Thread-Safety is Unknown)
 *
 */
ReadLineReturnValue readLine(int fd, byte *buf, unsigned long size, ReadBuf &cache) {
    assert(fd >= 3 && buf != nullptr && size > 0);
    auto isEndOfLine = [](const char *const ptr) {
        bool ans = true;
        for (unsigned long i = 0; i < SIZEOF_END_OF_LINE; i++) {
            ans = ans && (ptr[i] == END_OF_LINE[i]);
        }
        return ans;
    };
    ssize_t t = 0, cnt = 0;
    while (cnt < size && (t = readWithBuf(fd, buf, 1, cache)) > 0) {
        buf++;
        cnt++;
        if (cnt >= SIZEOF_END_OF_LINE && isEndOfLine(buf - SIZEOF_END_OF_LINE)) {
            *(buf - SIZEOF_END_OF_LINE) = 0;
            return {true, true, false, static_cast<unsigned long>(cnt - 2)};
        }
    }
    // error occur or EOF occur.
    // for that size>0, so the readWithBuf will be called at least one time
    if (t <= 0) {
        *buf = 0;
        // if errno==EBADF, EINVAL, EIO, then treat as EOF
        return {false, false, errno != EAGAIN && errno != EWOULDBLOCK, static_cast<size_t>(cnt)};
    }
    // when come here, means the size is run out
    assert(cnt == size);
    *buf = 0;
    return {false, false, false, static_cast<unsigned long>(cnt)};
}

/**
 * @return whether EOF occur
 * @warning
 * This function only work when the str doesn't contain alone CR or LR
 * @note
 * This method will block even the fd is nonblock
 *
 * MT-Safety: Unknown(because the `read` function's Thread-Safety is Unknown)
 */
bool consumeByteUntilEndOfLine(int fd, ReadBuf &cache) {
    // TODO, this function only work when the str doesn't contain alone CR or LR
    assert(fd >= 3);
    byte buf[10];
    while (readWithBuf(fd, buf, 1, cache) > 0
           || errno == EAGAIN || errno == EWOULDBLOCK) {
        if (buf[0] == END_OF_LINE[SIZEOF_END_OF_LINE - 1]) {
            return false;
        }
    }
    // error will be treated as EOF
    return true;
}

/**
 * @return whether get stat success.
 */
bool lstatWrap(const string &path, struct stat &statBuf) {
    if (lstat(path.c_str(), &statBuf) < 0) {
        if (errno == EBADE || errno == EFAULT
            || errno == EBADF || errno == EINVAL) {
            bugWithErrno("lstatWrap lstat failed", errno, true);
        } else {
            return false;
        }
    }
    return true;
}

bool statWrap(const string &path, struct stat &statBuf) {
    if (stat(path.c_str(), &statBuf) < 0) {
        if (errno == EBADE || errno == EFAULT
            || errno == EBADF || errno == EINVAL) {
            bugWithErrno("lstatWrap lstat failed", errno, true);
        } else {
            return false;
        }
    }
    return true;
}

bool isDir(struct stat &statBuf) {
    return (statBuf.st_mode & S_IFMT) == S_IFDIR;
}

bool euidAccessWrap(const string &path, int type) {
    if (euidaccess(path.c_str(), type) < 0) {
        if (errno == EFAULT) {
            bugWithErrno("euidAccessWrap euidaccess failed", errno, true);
        } else {
            return false;
        }
    }
    return true;
}

bool removeFile(const string &path) {
    if (remove(path.c_str()) < 0) {
        if (errno == EFAULT || errno == EBADF) {
            bugWithErrno("removeFile remove failed", errno, true);
        } else {
            return false;
        }
    }
    return true;
}

bool mkdirWrap(const string &path, mode_t mode) {
    if (mkdir(path.c_str(), mode) < 0) {
        if (errno == EFAULT || errno == EBADF) {
            bugWithErrno("mkdirWrap mkdir failed", errno, true);
        } else {
            return false;
        }
    }
    return true;
}

/**
 * @return whether get localtime success
 */
bool localtimeWrap(const time_t &time, struct tm &result) {
    localtime_r(&time, &result);
    if (errno == EOVERFLOW) {
        warningWithErrno("localtimeWrap localtime_r failed", errno);
        return false;
    }
    return true;
}

/**
 * @return return nullptr if error occur.
 * And errno is set appropriately
 */
DIR *openDirWrap(const string &path) {
    DIR *stream = opendir(path.c_str());
    if (stream == nullptr) {
        if (errno == EBADF || errno == ENOTDIR) {
            bugWithErrno("openDirWrap opendir failed: ", errno, true);
        } else {
            return nullptr;
        }
    }
    return stream;
}

void closeDirWrap(DIR *stream) {
    if (closedir(stream) < 0) {
        bugWithErrno("closeDirWrap closedir failed", errno, true);
    }
}

/**
 * @note
 * If failed, the errno will be set appropriately. errno can be ENOMEM ENOSPC.
 */
bool epollCtlWrap(int epollFd, int op, int fd, epoll_event *event = nullptr) {
    if (epoll_ctl(epollFd, op, fd, event) < 0) {
        if (errno == ENOMEM || errno == ENOSPC) {
            warningWithErrno("epollCtlWrap epoll_ctl failed", errno);
            return false;
        } else {
            bugWithErrno("epollCtlWrap epoll_ctl failed", errno, true);
        }
    }
    return true;
}

/**
 * @param timeout if -1, then wait indefinitely
 * @return The number of fd that is ready. If EINTR or timeout(no fd ready), return 0;
 */
int epollPWaitWrap(int epollFd, epoll_event events[],
                   int maxEvents, int timeout, const sigset_t &sigmask) {
    int r = epoll_pwait(epollFd, events, maxEvents, timeout, &sigmask);
    if (r < 0 && errno != EINTR) {
        bugWithErrno("epollPWaitWrap epoll_pwait failed", errno, true);
    } else {
        return r < 0 ? 0 : r;
    }
    assert(false);
}

/**
 * @param timeout if -1, then wait indefinitely
 * @return The number of fd that is ready. If EINTR or timeout(no fd ready), return 0;
 */
int epollWaitWrap(int epollFd, epoll_event events[], int maxEvents, int timeout) {
    int r = epoll_wait(epollFd, events, maxEvents, timeout);
    if (r < 0 && errno != EINTR) {
        bugWithErrno("epollPWaitWrap epoll_pwait failed", errno, true);
    } else {
        if (r < 0) {
            warningWithErrno("epollWaitWrap epoll_wait failed", errno);
            return 0;
        } else {
            return r;
        }
    }
    assert(false);
}

void signalWrap(int signum, sighandler_t handler) {
    if (signal(signum, handler) == SIG_ERR) {
        bugWithErrno("signalWrap signal failed", errno, true);
    }
}

#endif
