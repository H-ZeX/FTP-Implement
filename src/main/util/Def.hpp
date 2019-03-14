#ifndef __DEF_H__
#define __DEF_H__

#include <iostream>
#include <string>
#include <sys/types.h>
#include <exception>
#include <vector>

using std::string;
using std::vector;
using std::exception;

// the backtrace's buf size
#define BT_BUF_SIZE 1024
#define UNDETERMINED_LIMIT 0x1000
#define ERRNO_BUF_SIZE 256
#define WARNING_BUF_SIZE (2 * ERRNO_BUF_SIZE)
#define RECV_BUF_SIZE 1024
#define READ_BUF_SIZE 1024
#define DEFAULT_THREAD_CNT 100
#define MAX_TASK_CNT 10240
#define MAX_EPOLL_SIZE MAX_TASK_CNT
#define LOGIN_INCORRECT_DELAY_SEC 3
/**
 * according to manual of crypt's glibc note, The size of this string is fixed:
 * MD5:22 characters, SHA-256: 43 characters, SHA-512: 86 characters
 * the $6$salt$encrypted is an SHA-512 encoded one, "salt" stands for the up to 16  characters
 */
#define ENCRYPTED_KEY_MAX_LEN 256

/**
 * exit code
 */
#define BUG_EXIT 50
#define OPEN_MAINFD_ERROR 60
#define EPOLL_CREATE_ERROR 70
#define EPOLL_ADD_MAINFD_ERROR 80

/**
 * typedef
 */
#define errno_t int
#define ull_t unsigned long long
#define PBI pair<bool, int>
#define SA sockaddr
#define byte char
#define PII std::pair<int, int>
#define PBB std::pair<bool, bool>
#define PPI std::pair<PBB, int>
#define PSI std::pair<std::string, int>
#define CMD_MAX_LEN (PATH_MAX + 10)

class EndOfSessionException : public exception {
    const char *what() const noexcept override {
        return "EndOfSessionException";
    }
};

/**
 * product name and version
 */
#define PRODUCTION_NAME "hzxFTP"
#define PRODUCTION_VERSION "0.1"

/**
 * others
 */
#define END_OF_LINE "\r\n"
#define SIZEOF_END_OF_LINE (sizeof("\r\n") / sizeof(char) - 1)
#define BACK_LOG 20

#endif
