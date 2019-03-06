/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    utility.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-14-11:19:33
*   Describe:
*
**********************************************************************************/

#include "utility.h"

void bug(const char *const msg, bool willExit, int lineNum) {
    void *buffer[BT_BUF_SIZE];
    int t = backtrace(buffer, BT_BUF_SIZE);
    char **str = backtrace_symbols(buffer, t);
    if (str == NULL) {
        fprintf(stderr, "backtrace_symbols error");
        exit(BUG_EXIT);
    }
    flockfile(stderr);
    fprintf(stderr, "%s\nLineNumber: %d\n", msg, lineNum);
    for (int i = 0; i < t; i++) {
        fprintf(stderr, "%s\n", str[i]);
    }
    funlockfile(stderr);
    free(str);
    if (willExit) {
        exit(BUG_EXIT);
    }
}
void fatalError(const char *const msg) {
    fprintf(stderr, "%s\n", msg);
    exit(FATAL_ERROR_EXIT);
}

void bugWithErrno(const char *const msg, errno_t code, bool willExit) {
    char errnoBuf[ERRNO_BUF_SIZE], warningBuf[WARNING_BUF_SIZE];
    if (snprintf(warningBuf, WARNING_BUF_SIZE, "%s: %s", msg,
                 strerror_r(code, errnoBuf, ERRNO_BUF_SIZE)) >= WARNING_BUF_SIZE) {
        bug("bugWithErrno snprintf buffer size too small", false);
    }
    bug(warningBuf, willExit);
}

void warning(const char *const msg) {
    fprintf(stderr, "%s\n", msg);
}
void warningWithErrno(const char *const msg, errno_t code) {
    char errnoBuf[ERRNO_BUF_SIZE], warningBuf[WARNING_BUF_SIZE];
    if (snprintf(warningBuf, WARNING_BUF_SIZE, "%s: %s", msg,
                 strerror_r(code, errnoBuf, ERRNO_BUF_SIZE)) >= WARNING_BUF_SIZE) {
        bug("warningWithErrno snprintf buffer size too small", false);
    }
    warning(warningBuf);
}

void mylog(const char *const msg) {
    printf("%s\n", msg);
    fflush(stdout);
}

long Sysconf(int name) {
    errno_t es = errno;
    errno = 0;
    long t = sysconf(name);
    char errnoBuf[ERRNO_BUF_SIZE];
    if (errno) {
        bug(strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE));
    }
    errno = es;
    return t;
}
/**
 * @return the limit on the number of thread
 * for the real user ID of the calling process.
 */
size_t maxThreadCnt() {
    rlimit rl{};
    if (getrlimit(RLIMIT_NPROC, &rl) < 0) {
        return DEFAULT_THREAD_CNT;
    } else {
        return std::min<rlim_t>(rl.rlim_cur, rl.rlim_max);
    }
}

char *mereString(char *dest, const char *const strArray[], size_t maxLen) {
    size_t restLen = maxLen;
    dest[0] = 0;
    for (int i = 0; strArray[i]; i++ && restLen > 0) {
        strncat(dest, strArray[i], restLen);
        restLen -= strnlen(strArray[i], restLen);
    }
    return dest;
}

void string_split(const std::string &str, std::vector<std::string> &res, char separate,
                  bool clearRes) {
    std::vector<char> sp;
    if (separate == 0) {
        sp.push_back(' ');
        sp.push_back('\f');
        sp.push_back('\n');
        sp.push_back('\r');
        sp.push_back('\t');
        sp.push_back('\v');
    } else {
        sp.push_back(separate);
    }
    auto judge = [sp](unsigned char c) {
        for (auto it = sp.begin(); it != sp.end(); ++it) {
            if (*it == c) {
                return true;
            }
        }
        return false;
    };
    if (clearRes) {
        res.clear();
    }
    int p = 0, q = 0;
    auto it = str.begin();
    while (it != str.end()) {
        while (it != str.end() && judge(*it)) {
            p++, q++, ++it;
        }
        while (it != str.end() && !judge(*it)) {
            q++, ++it;
        }
        if (p != q) {
            res.push_back(str.substr(p, q - p));
        }
        p = q;
    }
}
void string_upper(char *src, int len) {
    for (int i = 0; i < len; i++) {
        src[i] = (src[i] >= 'a' && src[i] <= 'z') * (-0x20) + src[i];
    }
}
char char_upper(char c) {
    return (c >= 'a' && c <= +'z') * (-0x20) + c;
}
bool isNumber(const char *const str, int base) {
    for (int i = 0; str[i]; i++) {
        if (base <= 10) {
            if (str[i] >= '0' + base && str[i] < '0') {
                return false;
            }
        } else {
            if (!isalnum(str[i]) || (str[i] >= 'a' + base - 10 && str[i] >= 'A' + base - 10)) {
                return false;
            }
        }
    }
    return str[0] != 0;
}

byte *byteCpy(byte *dest, const byte *const src, int n) {
    for (int i = 0; i < n; i++) {
        dest[i] = src[i];
    }
    return dest;
}
byte *byteCat(byte *dest, int ndest, const byte *const src, int nsrc) {
    dest += ndest;
    for (int i = 0; i < nsrc; i++) {
        dest[i] = src[i];
    }
    return dest - ndest;
}
int readWithBuf(int fd, byte *result, int want, ReadBuf &buf) {
    if (buf.reRCap > 0) {
        int t = std::min<int>(buf.reRCap, want);
        byteCpy(result, buf.readPtr, t);
        buf.reRCap -= t;
        if (buf.reRCap == 0) {
            buf.readPtr = buf.buf;
        } else {
            buf.readPtr += t;
        }
        return t;
    }
    int cnt = -1;
    while (cnt < 0) {
        cnt = read(fd, buf.buf, sizeof(buf.buf));
        if (cnt < 0 && errno != EINTR) {
            warningWithErrno("readWithBuf read()");
            return READ_FAIL;
        } else if (errno == EINTR) {
            continue;
        } else if (cnt == 0) {
            return 0;
        } else if (cnt > 0) {
            buf.reRCap += cnt;
            int t = std::min<int>(buf.reRCap, want);
            byteCpy(result, buf.readPtr, t);
            buf.reRCap -= t;
            if (buf.reRCap == 0) {
                buf.readPtr = buf.buf;
            } else {
                buf.readPtr += t;
            }
            return t;
        }
    }
    // should not come here;
    return READ_FAIL;
}
bool writeAllData(int fd, const byte *buf, size_t size) {
    // If size is zero and fd refers to a file other than a regular file, the results of write are
    // not specified.
    if (size == 0) {
        return true;
    }
    int r;
    while (size > 0) {
        r = write(fd, buf, size);
        if (r < 0 && errno != EINTR) {
            warningWithErrno("writeAllData write()");
            return false;
        } else if (r == 0) { // if no this case, the loop may cycle forever
            warning("writeAllData write() return 0, write failed");
            return false;
        } else if (errno != EINTR) {
            size -= r;
            buf += r;
        }
    }
    fprintf(stderr, "writeAllData true\n");
    return true;
}
bool closeFileDescriptor(int fd, const char *msg) {
    if(fd<3 && fd>=0) {
        bug("closeFileDescriptor");
    }
    errno_t ppp[] = {EINTR, 0};
    int r;
    errnoRetryV_1(close(fd), ppp, "closeFileDescriptor", r);
    if (r < 0) {
        warningWithErrno((string("closeFileDescriptor failed") + msg).c_str());
    }
    return r == 0;
}

bool checkFdType(int fd, int type, const char *const errorMsg, const char *const failMsg) {
    int t = isfdtype(fd, type);
    cout << t << endl;
    if (t == 1) {
        return true;
    } else if (t == 0) {
        bug(failMsg);
        return false;
    } else {
        warning((string(errorMsg) + ": checkFdType failed").c_str());
        return true;
    }
}
UserInfo getUidGidHomeDir(const char *const username) {
    struct passwd pwd, *res;
    long t = Sysconf(_SC_GETPW_R_SIZE_MAX);
    t = (t == -1) * UN_DETERMINATE_LIMIT + (t != -1) * t;
    char buf[t];
    errno_t ea[] = {EINTR, 0};
    int q;
    errnoRetryV_2(getpwnam_r(username, &pwd, buf, t, &res), ea, "getUidGid failed", q);
    if (q != 0) {
        warningWithErrno("getUidGid failed", q);
    }
    return UserInfo(q == 0 && res != NULL, pwd.pw_uid, pwd.pw_gid, "", "", username, pwd.pw_dir);
}

bool setThreadUidAndGid(uid_t uid, gid_t gid) {
    if (syscall(SYS_setgid, gid) != 0) {
        warningWithErrno("setThreadUid");
        return false;
    }
    if (syscall(SYS_setuid, uid) != 0) {
        warningWithErrno("setThreadUid");
        return false;
    }
    return true;
}

std::string &constructMsg(const char *src[], std::string &dest, bool willClearDest) {
    if (willClearDest) {
        dest.clear();
    }
    int len = 0;
    for (int i = 0; src[i]; i++) {
        int sl = strnlen(src[i], MSG_MAX_LEN);
        for (int j = 0; j < sl; j++) {
            dest.push_back(src[i][j]);
            len++;
            if (len % (MCEL_WITHOUT_EOL) == 0) {
                for (int k = 0; k < SIZEOF_END_OF_LINE; k++) {
                    dest.push_back(END_OF_LINE[k]);
                }
            }
        }
    }
    if ((*dest.rbegin()) == END_OF_LINE[SIZEOF_END_OF_LINE - 1]) {
        return dest;
    } else {
        for (int k = 0; k < SIZEOF_END_OF_LINE; k++) {
            dest.push_back(END_OF_LINE[k]);
        }
        return dest;
    }
}

/**
 * @return (cmd, paramIndex)
 * paramIndex is the index to @param param which is the beginning of cmdParam
 */
PSI parseCmd(const char *param) {
    int i = 0;
    while (param[i] && isspace(param[i])) {
        i++;
    }
    if (param[i] == 0) {
        return PSI("INVALID", 0);
    }
    string cmd;
    while (param[i] && isalpha(param[i])) {
        cmd.push_back(char_upper(param[i]));
        i++;
    }
    while (param[i] && isspace(param[i])) {
        i++;
    }
    return PSI(cmd, i);
}

bool setNonBlocking(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        warningWithErrno("setNonBlocking");
        return false;
    }
    return true;
}

bool setBlocking(int fd) {
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & (~O_NONBLOCK)) == -1) {
        warningWithErrno("setBlocking");
        return false;
    }
    return true;
}
bool isBlocking(int fd) {
    int t = 0;
    if ((t = fcntl(fd, F_GETFL, 0)) == -1) {
        warningWithErrno("setBlocking");
        return false;
    }
    return !(t & O_NONBLOCK);
}

bool wrapForSignalAction(int signum, sighandler_t handler) {
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* Block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* Restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0) {
        warningWithErrno("wrapForSignalAction sigaction");
        return false;
    }
    return true;
    //    return (old_action.sa_handler);
}
