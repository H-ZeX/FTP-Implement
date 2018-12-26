/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    utility.h
*   Author:       H-ZeX
*   Create Time:  2018-08-14-11:19:33
*   Describe:
*
**********************************************************************************/
#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "def.h"
#include "utility.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <grp.h>
#include <linux/limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <semaphore.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <iostream>
using namespace std;

/**
 * same as sysconf except if errno==EINVAL, call bug()
 * @return -1 if name is option and unsupported or name is limit and undeterminate
 * @return otherwise return the result
 */
long Sysconf(int name);
/**
 * @return if get system's conf fail, return the DEFAULT_THREAD_CNT
 */
size_t maxThreadCnt();
/**
 * mere all string in strArray and save in dest,
 * the end of strArray must be (char*)NULL
 * the str saved in dest will be at most maxlen(without \0)
 */
char *mereString(char *dest, const char *const strArray[], size_t maxLen);

/**
 * print the error msg and backtrace then exit,
 * exit code is BUG_EXIT
 */
void bug(const char *const msg, bool willExit = true, int lineNum = -1);
void bugWithErrno(const char *const msg, errno_t code, bool willExit = true);
void warning(const char *const msg);
void warningWithErrno(const char *const msg, errno_t code = errno);
void mylog(const char *const msg);
void fatalError(const char *const msg);

/**
 * calculate the length of num
 */
template <typename T> int numLen(T num, int base) {
    int ans = 0;
    while (num) {
        num /= base;
        ans++;
    }
    return ans;
}

/**
 * @param clearRes whether or not clear the res vector
 */
void string_split(const std::string &str, std::vector<std::string> &res, char separate = 0,
                  bool clearRes = true);
/**
 * convert the src[i] to uppercase
 */
void string_upper(char *src, int len);

char char_upper(char c);

/**
 * judege whether a str is number wich base is @param base
 * the base must be 1 to 36, or will be undefined
 */
inline bool isNumber(const char *const str, int base);

/**
 * for function who would like to retry for some errno
 * @param errnoArray is an errno_t array whose last element is 0
 * if the errnoArray[0]==0, then the action is unspecified
 */
#define errnoRetryV_1(action, errnoArray, warningMsg, returnCode)                                  \
    do {                                                                                           \
        auto shouldContinue = [errnoArray](int code) {                                             \
            bool c = false;                                                                        \
            for (int i = 0; errnoArray[i]; i++) {                                                  \
                c = c || (code == errnoArray[i]);                                                  \
            }                                                                                      \
            return c;                                                                              \
        };                                                                                         \
        int cnt = 0;                                                                               \
        do {                                                                                       \
            returnCode = action;                                                                   \
            cnt++;                                                                                 \
            if (cnt >= RETRYTIME) {                                                                \
                break;                                                                             \
            }                                                                                      \
        } while (shouldContinue(errno));                                                           \
        if (shouldContinue(errno)) {                                                               \
            warningWithErrno(warningMsg);                                                          \
        }                                                                                          \
    } while (0)
/**
 * same as errnoRetryV_1 except use the returnCode as errno
 */
#define errnoRetryV_2(action, errnoArray, warningMsg, returnCode)                                  \
    do {                                                                                           \
        auto shouldContinue = [errnoArray](int code) {                                             \
            bool c = false;                                                                        \
            for (int i = 0; errnoArray[i]; i++) {                                                  \
                c = c || (code == errnoArray[i]);                                                  \
            }                                                                                      \
            return c;                                                                              \
        };                                                                                         \
        int cnt = 0;                                                                               \
        do {                                                                                       \
            returnCode = action;                                                                   \
            cnt++;                                                                                 \
            if (cnt >= RETRYTIME) {                                                                \
                break;                                                                             \
            }                                                                                      \
        } while (shouldContinue(returnCode));                                                      \
        if (shouldContinue(returnCode)) {                                                          \
            warningWithErrno(warningMsg, returnCode);                                              \
        }                                                                                          \
    } while (0)

#define WrapForFunc(action, successCode, msg, isUseErrno, returnCode)                              \
    do {                                                                                           \
        if ((returnCode = action) != successCode) {                                                \
            if (isUseErrno) {                                                                      \
                warningWithErrno(msg, errno);                                                      \
            } else {                                                                               \
                warningWithErrno(msg, returnCode);                                                 \
            }                                                                                      \
        }                                                                                          \
    } while (0);

byte *byteCpy(byte *dest, const byte *const src, int n);
byte *byteCat(byte *dest, int ndest, const byte *const src, int nsrc);

/**
 * the result should have enough size, or the behavious is undefined
 * @return READ_FAIL failed
 * @return the size been read, 0 indicate EOF
 *
 * MT-safe
 */
int readWithBuf(int fd, byte *result, int want, ReadBuf &buf);

/**
 *  @return whether write All data success
 *
 *  MT-safe since linux 3.14
 */
bool writeAllData(int fd, const byte *buf, size_t size);

/**
 * @return whether close success
 *
 * MT-safe
 */
bool closeFileDescriptor(int fd, const char *msg = "");

/**
 * @return true the fd is of type @param type or error occure
 * @return false for that bug is called, no false will be returned
 *
 * MT-safe
 */
bool checkFdType(int fd, int type, const char *const errorMsg, const char *const failMsg);

/**
 * @return UserInfo.isValid don't means whether the user and pwd is valid
 * but means that whether getUidGidHomeDir success;
 */
UserInfo getUidGidHomeDir(const char *const username);

/**
 * @return whether succeed
 */
bool setThreadUidAndGid(uid_t uid, gid_t gid);
/**
 * @param src, the last element is 0 indicate the end of src
 * if src[i] is longer than MSG_MAX_LEN, the rest of it will not be use
 * the src[i] must be str(the last char is 0), or lead to may buffer overflow
 * @param dest will be clear if @param willClearDest is true
 *
 * MT-safe
 */
std::string &constructMsg(const char *src[], std::string &dest, bool willClearDest = true);
/**
 * @return (cmd, paramIndex)
 * paramIndex is the index to @param param which is the beginning of cmdParam
 *
 * MT-safe
 */
PSI parseCmd(const char *param);

bool setNonBlocking(int fd);
bool setBlocking(int fd);
bool isBlocking(int fd);
bool wrapForSignalAction(int signum, sighandler_t handler);

#endif
