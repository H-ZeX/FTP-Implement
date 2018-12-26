/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    netUtility.h
*   Author:       H-ZeX
*   Create Time:  2018-08-19-09:54:19
*   Describe:
*
**********************************************************************************/

#ifndef __NET_UTILITY_H__
#define __NET_UTILITY_H__

#include "def.h"
#include "utility.h"
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * MT-safe env local
 * @return if success, return file descriptor
 * @return -6 accept connect fail
 *
 * MT-safe env local
 */
int acceptConnect(int listenfd);
/**
 * MT-safe  env local
 * @return -5 close failed (the function inside also use close)
 * @return -4 open_clientfd failed
 * @return -2 getaddrinfo failed
 * @return if succes, return file descriptor
 *
 * MT-safe env local
 */
int openClientfd(const char *const hostname, const char *const port);
/**
 * @return -3 open_listenfd failed
 * @return -2 getaddrinfo failed
 * @return -1 close failed(the function inside also use close)
 * @return if succes, return file descriptor
 *
 * MT-safe  env local
 */
int openListenfd(const char *const port, int listenq = LISTENQ);
/**
 * @return ((isEND_OF_LINE, isEOF), recvCnt)
 * @return if recvCnt<0, error occure
 *
 * MT-safe
 */
PPI recvline(int fd, byte *buf, size_t size, ReadBuf &cache);
PPI recvlineWithoutBuf(int fd, char *buf, size_t size);

/**
 * MT-safe
 * @return true occure to EOF
 */
bool recvToEOL(int fd, ReadBuf &cache);
/**
 * @return the result
 * if faild, return empty string
 * MT-safe env local
 */
const string getThisMachineIp();

#endif
