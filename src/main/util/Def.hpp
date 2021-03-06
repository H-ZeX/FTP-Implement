#ifndef __DEF_H__
#define __DEF_H__

#include <iostream>
#include <string>
#include <sys/types.h>
#include <exception>
#include <vector>

// the backtrace's buf size
#define BT_BUF_SIZE 1024
#define UNDETERMINED_LIMIT 0x1000
#define ERRNO_BUF_SIZE 256
#define WARNING_BUF_SIZE (2 * ERRNO_BUF_SIZE)
#define RECV_BUF_SIZE 1024
#define READ_BUF_SIZE 1024
/*
 * The FTP_MAX_USER_ONLINE_CNT should <= THREAD_POOL_MAX_TASK_CNT
 */

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

#endif
