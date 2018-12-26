/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    def.h
*   Author:       H-ZeX
*   Create Time:  2018-08-15-10:01:14
*   Describe:
**********************************************************************************/

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

#define BT_BUF_SIZE 100 // the backtrace's buf size
#define UNDETERMINATE_LIMIT 0x1000
#define ERRNO_BUF_SIZE 256
#define WARNING_BUF_SIZE (2 * ERRNO_BUF_SIZE)
#define HOSTNAME_MAX_LEN 256
#define PORT_MAX_LEN 20
#define RECV_BUF_SIZE 1024
#define READ_BUF_SIZE 1024
#define DEFAULT_THREAD_CNT 100
#define MAX_TASK_CNT 10240
#define MUTEX_TIMEOUT_SEC 600
#define MAX_EPOLL_SIZE MAX_TASK_CNT
#define LOGIN_INCORRECT_DELAY_SEC 3
/**
 * according to manual of crypt's glibc note, The size of this string is fixed:
 * MD5:22 characters, SHA-256: 43 characters, SHA-512: 86 characters
 * the $6$salt$encrypted is an SHA-512 encoded one, "salt" stands for the up to 16  characters
 */
#define ENCRYPTED_KEY_MAX_LEN 256
#define MSG_MAX_LEN 8096

/**
 * exit code
 */
#define BUG_EXIT 50
#define OPEN_MAINFD_ERROR 60
#define EPOLL_CREATE_ERROR 70
#define EPOLL_ADD_MAINFD_ERROR 80
#define EPOLL_WAIT_ERROR 90
#define FATAL_ERROR_EXIT 110

/**
 * return code
 */
#define READ_FAIL -8
#define ACCEPT_CONNECT_FAIL -6
#define OPEN_CLIENTFD_CLOSE_FAIL -5
#define OPEN_CLIENTFD_FAIL -4
#define OPEN_LISTENFD_FAIL -3
#define GET_ADDR_INFO_FAIL -2
#define OPEN_LISTENFD_CLOSE_FAIL -1
#define SUCCESS 0
#define OPENDIR_FAIL 1
#define READDIR_FAIL 2
#define STAT_FAIL 3
/**
 * typedef
 */
#define errno_t int
#define ull_t unsigned long long
#define PBI pair<bool, int>
#define SA sockaddr
#define byte char
#define PBB std::pair<bool, bool>
#define PPI std::pair<PBB, int>
#define PSI std::pair<std::string, int>
#define CMD_MAX_LEN (PATH_MAX + 10)

struct task_t {
    void *(*func)(void *);

    void *argument;

    void (*callback)(void *);

    task_t() {
        this->func = nullptr;
        this->argument = nullptr;
        this->callback = nullptr;
    }

    explicit task_t(void *(*func)(void *), void *argument = nullptr, void (*callback)(void *) = nullptr) {
        this->func = func;
        this->argument = argument;
        this->callback = callback;
    }

    bool isValid() const {
        return this->func;
    }
};

struct UserInfo {
    bool isValid;
    uid_t uid;
    gid_t gid;
    string cmdIp;
    string cmdPort;
    string username;
    string homeDir;

    UserInfo() {
        this->isValid = false;
        this->uid = static_cast<uid_t>(-1);
        this->gid = static_cast<gid_t>(-1);
    }

    UserInfo(bool is, uid_t u, gid_t g, const string &ip, const string &port, const string &user,
             const char *const hd) {
        this->isValid = is;
        this->uid = u;
        this->gid = g;
        this->cmdIp = ip;
        this->cmdPort = port;
        this->username = user;
        this->homeDir = hd;
    }
};

struct ReadBuf {
    byte buf[READ_BUF_SIZE]{};
    byte *readPtr; // point to the start of haven't read substr
    int reRCap; //  reRCap is remainder read capacity
    ReadBuf() {
        this->readPtr = buf;
        this->reRCap = 0;
    }
};

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
#define RETRYTIME 5
#define END_OF_LINE "\r\n"
#define SIZEOF_END_OF_LINE (sizeof("\r\n") / sizeof(char) - 1)
// max char cnt each line without eol
#define MCEL_WITHOUT_EOL (80 - SIZEOF_END_OF_LINE)
#define LISTENQ 10240

#endif
