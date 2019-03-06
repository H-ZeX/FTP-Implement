/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    netUtility.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-19-09:54:25
*   Describe:
*
**********************************************************************************/
#include "netUtility.h"
#include <iostream>
using namespace std;

/**
 * MT-safe env local
 *
 * @return if success, return file descriptor
 * @return -6 accept connect fail
 */
int acceptConnect(int listenfd) {
    char hostname[HOSTNAME_MAX_LEN], port[PORT_MAX_LEN];
    sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int connfd;
    int ppp[] = {EINTR, 0};
    errnoRetryV_1(accept(listenfd, (SA *)&clientaddr, &clientlen), ppp, "acceptConnect failed",
                  connfd);
    if (connfd < 0) {
        warningWithErrno("acceptConnect failed");
        return ACCEPT_CONNECT_FAIL;
    }
    // getnameinfo MT-safe env local
    int r = getnameinfo((SA *)&clientaddr, clientlen, hostname, HOSTNAME_MAX_LEN, port,
                        PORT_MAX_LEN, NI_NUMERICHOST);
    if (r != 0) {
        warningWithErrno("accepted connection successfully, get name info fail");
    } else {
        mylog(("accepted connect from (" + std::string(hostname) + ", " + std::string(port) + ")")
                  .c_str());
    }
    return connfd;
}

/**
 * @return -5 close failed (the function inside also use close)
 * @return -4 open_clientfd failed
 * @return -2 getaddrinfo failed
 * @return if succes, return file descriptor
 *
 * MT-safe  env local
 */
int openClientfd(const char *const hostname, const char *const port) {
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;
    char errnoBuf[ERRNO_BUF_SIZE];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        sprintf(errnoBuf, "getaddrinfo failed (port %s): %s", port, gai_strerror(rc));
        warning(errnoBuf);
        return GET_ADDR_INFO_FAIL;
    }
    for (p = listp; p; p = p->ai_next) {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }
        errno_t ppp[] = {EINTR, 0};
        int r;
        errnoRetryV_1(connect(clientfd, p->ai_addr, p->ai_addrlen), ppp, "open_clientfd", r);
        if (r == 0) {
            break;
        } else {
            warningWithErrno("open_clientfd connect() failed");
        }
        errnoRetryV_1(close(clientfd), ppp, "open_listenfd", r);
        if (r < 0) {
            warningWithErrno("open_listenfd close() failed");
        }
    }
    freeaddrinfo(listp);
    if (!p) {
        warning(("open client file descriptor on " + std::string(hostname) + " : " +
                 std::string(port) + " failed")
                    .c_str());
        return OPEN_CLIENT_FD_FAIL;
    } else {
        return clientfd;
    }
}

/**
 * @return -3 open_listenfd failed
 * @return -2 getaddrinfo failed
 * @return -1 close failed(the function inside also use close)
 * @return if succes, return file descriptor
 *
 * MT-safe  env local
 */
int openListenfd(const char *const port, int listenq) {
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval = 1;
    char tmpBuf[ERRNO_BUF_SIZE];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    // getaddrinfo is MT-safe env local
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        sprintf(tmpBuf, "open_listenfd getaddrinfo() failed (port %s): %s", port, gai_strerror(rc));
        warning(tmpBuf);
        return GET_ADDR_INFO_FAIL;
    }
    for (p = listp; p; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        if (close(listenfd) < 0) {
            warningWithErrno("open_listenfd close() failed");
            return OPEN_CLIENT_FD_CLOSE_FAIL;
        }
    }
    freeaddrinfo(listp);
    if (!p) {
        warning(("open listen file descriptor on port " + std::string(port) + " failed").c_str());
        return OPEN_LISTEN_FD_FAIL;
    }
    if (listen(listenfd, listenq) < 0) {
        warningWithErrno("open_listenfd listen() failed");
        int r;
        errno_t ppp[] = {EINTR, 0};
        errnoRetryV_1(close(listenfd), ppp, "open_listenfd close() failed", r);
        if (r < 0) {
            warningWithErrno("open_listenfd close() failed");
        }
        return OPEN_LISTEN_FD_CLOSE_FAIL;
    }
    return listenfd;
}

/**
 * @return ((isEND_OF_LINE, isEOF), recvCnt)
 * @return if recvCnt<0, error occure
 * the buf will not end with END_OF_LINE and  will end with 0
 */
PPI recvline(int fd, char *buf, size_t size, ReadBuf &cache) {
    char *const bufPtr = buf; // record the original ptr of buf
    checkFdType(fd, S_IFSOCK, "recvline", "pass not socket fd to recvline");
    auto isEOL = [](const char *const ptr) {
        bool ans = true;
        for (int i = 0; i < SIZEOF_END_OF_LINE; i++) {
            ans = ans && (ptr[i] == END_OF_LINE[i]);
        }
        return ans;
    };
    int t = 0, cnt = 0;
    while (cnt < size && ((t = readWithBuf(fd, buf, 1, cache)) > 0 || errno == EINTR ||
                          errno == EAGAIN || errno == EWOULDBLOCK)) {
        ++buf;
        cnt++;
        if (cnt >= SIZEOF_END_OF_LINE && isEOL(buf - SIZEOF_END_OF_LINE)) {
            *(buf - SIZEOF_END_OF_LINE) = 0;
            fprintf(stderr, "recvline %s\n", bufPtr);
            return PPI(PBB(true, false), cnt - SIZEOF_END_OF_LINE);
        }
    }
    if (t < 0) {
        warningWithErrno("recvline");
    }
    *bufPtr = 0;
    fprintf(stderr, "recvline(None)");
    return PPI(PBB(false, t <= 0), 0);
}
/**
 * @return ((isEND_OF_LINE, EOF), cnt)
 */
PPI recvlineWithoutBuf(int fd, char *buf, size_t size) {
    checkFdType(fd, S_IFSOCK, "recvlineWithoutBuf", "pass not socket fd to recvlineWithoutBuf");
    char *const bufPtr = buf;
    auto isEOL = [](const char *const ptr) {
        bool ans = true;
        for (int i = 0; i < SIZEOF_END_OF_LINE; i++) {
            ans = ans && (ptr[i] == END_OF_LINE[i]);
        }
        return ans;
    };
    int cnt = 0, t;
    while (cnt < size && ((t = read(fd, buf + cnt, 1)) > 0 || errno == EINTR || errno == EAGAIN ||
                          errno == EWOULDBLOCK)) {
        cnt += (t == 1);
        if (cnt >= SIZEOF_END_OF_LINE && isEOL(buf + cnt - SIZEOF_END_OF_LINE)) {
            *(buf - SIZEOF_END_OF_LINE) = 0;
            fprintf(stderr, "recvlineWithoutBuf %s\n", bufPtr);
            return PPI(PBB(true, false), cnt - SIZEOF_END_OF_LINE);
        }
    }
    if (t < 0) {
        warningWithErrno("recvlineWithoutBuf");
    }
    *bufPtr = 0;
    fprintf(stderr, "recvlineWithoutBuf(nothing) \n", bufPtr);
    return PPI(PBB(false, t<=0), 0);
}

/**
 * @return isEOF
 * TODO the way of checking whether EOL is not very good
 */
bool recvToEOL(int fd, ReadBuf &cache) {
    checkFdType(fd, S_IFSOCK, "recvToEOL", "pass not sock fd to recvToEOL");
    byte buf[10];
    int t;
    while ((t = readWithBuf(fd, buf, 1, cache)) > 0) {
        if (buf[0] == END_OF_LINE[SIZEOF_END_OF_LINE - 1]) {
            return false;
        }
    }
    return t <= 0;
}

const string getThisMachineIp() {
    std::string res;
    char line[100], *p, *c, *save_1;
    FILE *f = fopen("/proc/net/route", "r");
    while (fgets(line, 100, f)) {
        p = strtok_r(line, " \t", &save_1);
        c = strtok_r(NULL, " \t", &save_1);
        if (p != NULL && c != NULL) {
            if (strncmp(c, "00000000", 8) == 0) {
                break;
            }
        }
    }
    int fm = AF_INET, family;
    struct ifaddrs *ifaddr;
    char host[NI_MAXHOST], buf[ERRNO_BUF_SIZE];
    if (getifaddrs(&ifaddr) == -1) {
        warningWithErrno("getThisMachineIp getifaddrs() failed");
        res.clear();
        return res;
    }
    for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }
        family = ifa->ifa_addr->sa_family;
        if (strcmp(ifa->ifa_name, p) != 0 || family != fm) {
            continue;
        }
        int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0,
                            NI_NUMERICHOST);
        if (s != 0) {
            snprintf(buf, ERRNO_BUF_SIZE, "getThisMachineIp getnameinfo() failed: %s",
                     gai_strerror(s));
            warning(buf);
            res.clear();
            return res;
        } else {
            res = host;
            break;
        }
    }
    freeifaddrs(ifaddr);
    return res;
}
