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
#define THREAD_POOL_MAX_TASK_CNT 10240
/*
 * The FTP_MAX_USER_ONLINE should <= THREAD_POOL_MAX_TASK_CNT
 */
#define FTP_MAX_USER_ONLINE 10200
#define LOGIN_INCORRECT_DELAY_SEC 3

/**
 * exit code
 */
#define BUG_EXIT 50

/**
 * typedef
 */
#define errno_t int
#define ull_t unsigned long long
#define SA sockaddr
#define byte char
#define PBB std::pair<bool, bool>
#define PSI std::pair<std::string, int>

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
