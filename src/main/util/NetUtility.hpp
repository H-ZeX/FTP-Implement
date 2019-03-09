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

#include "Def.hpp"
#include <cerrno>
#include "src/main/util/utility.hpp"
#include <ifaddrs.h>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <cassert>
#include <fcntl.h>

/**
 * @return if success, return file descriptor.
 * @return -1 accept connect fail
 *
 * MT-Safety: Unknown(the `makeSureFdType` and `accept` function's Thread-Safety is Unknown)
 */
int acceptConnect(int listenFd) {
    assert(listenFd >= 3);
    makeSureFdType(listenFd, S_IFSOCK);
    sockaddr_storage clientAddr{};
    socklen_t clientAddrSize = sizeof(clientAddr);
    do {
        int fd = accept(listenFd, (SA *) &clientAddr, &clientAddrSize);
        // TODO need to refine the errno handler
        if (errno != EINTR) {
            return fd;
        }
    } while (errno == EINTR);
    assert(false);
}

/**
 * @return if success, return file descriptor
 * @return -1 if failed
 *
 * MT-Safety: Unknown(Because the `socket`, `connect`, `close` function's Thread-Safety is Unknown)
 */
int openClientFd(const char *hostname, const char *port) {
    assert(hostname != nullptr && port != nullptr);
    int clientFd = 0, rc;
    addrinfo hints{}, *list;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    if ((rc = getaddrinfo(hostname, port, &hints, &list)) != 0) {
        warningWithErrno("openClientFd getaddrinfo failed", (rc == EAI_SYSTEM) ? errno : rc);
        return -1;
    }
    addrinfo *p = nullptr;
    for (p = list; p; p = p->ai_next) {
        if ((clientFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }
        int r = connect(clientFd, p->ai_addr, p->ai_addrlen);
        if (r == 0) {
            break;
        } else {
            // TODO errno handler should be refined
            warningWithErrno("openClientFd connect failed", errno);
        }
        closeFileDescriptor(clientFd);
    }
    freeaddrinfo(list);
    if (!p) {
        warning(("open client file descriptor on " + std::string(hostname) + " : " +
                 std::string(port) + " failed").c_str());
        return -1;
    } else {
        return clientFd;
    }
}

/**
 * @return if success, return file descriptor
 * @return -1 if failed
 *
 * MT-Safety: Unknown(Because the `socket`, `connect`, `close`,
 * `listen`, `bind` function's Thread-Safety is Unknown)
 */
int openListenFd(const char *port, int backLog = BACK_LOG) {
    assert(port != nullptr && backLog > 0);
    addrinfo hints{}, *list;
    int listenFd = 0, opt = 1;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    // getaddrinfo is MT-safe env local
    int rc = 0;
    if ((rc = getaddrinfo(nullptr, port, &hints, &list)) != 0) {
        warningWithErrno("openClientFd getaddrinfo failed",
                         rc == EAI_SYSTEM ? errno : rc);
        return -1;
    }
    addrinfo *p = nullptr;
    for (p = list; p; p = p->ai_next) {
        if ((listenFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }
        int r = setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(int));
        if (r != 0) {
            continue;
        }
        if (bind(listenFd, p->ai_addr, p->ai_addrlen) == 0) {
            break;
        }
        closeFileDescriptor(listenFd);
    }
    freeaddrinfo(list);
    if (!p) {
        warning(("open listen file descriptor on port " + std::string(port) + " failed").c_str());
        return -1;
    }
    if (listen(listenFd, backLog) < 0) {
        // TODO errno handler should be refined
        warningWithErrno("openListenFd listen failed", errno);
        return -1;
    }
    return listenFd;
}

/**
 * @return the result
 * if failed, return empty string
 * MT-safe env local
 */
const string getThisMachineIp() {
    char line[100], *p, *c, *save;
    FILE *routeFile = fopen("/proc/net/route", "r");
    while (fgets(line, 100, routeFile)) {
        p = strtok_r(line, " \t", &save);
        c = strtok_r(nullptr, " \t", &save);
        if (p != nullptr && c != nullptr) {
            if (strncmp(c, "00000000", 8) == 0) {
                break;
            }
        }
    }
    int fm = AF_INET, family;
    ifaddrs *addr;
    char host[NI_MAXHOST];
    if (getifaddrs(&addr) == -1) {
        // TODO errno handler should be refined
        warningWithErrno("getThisMachineIp getifaddrs failed", errno);
        return "";
    }
    for (ifaddrs *ifa = addr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }
        family = ifa->ifa_addr->sa_family;
        if (strcmp(ifa->ifa_name, p) != 0 || family != fm) {
            continue;
        }
        int s = getnameinfo(ifa->ifa_addr,
                            sizeof(struct sockaddr_in),
                            host, NI_MAXHOST,
                            nullptr, 0,
                            NI_NUMERICHOST);
        if (s != 0) {
            // TODO errno handler should be refined
            warningWithErrno("getThisMachineIp getnameinfo failed", s == EAI_SYSTEM ? errno : s);
            return "";
        } else {
            break;
        }
    }
    freeifaddrs(addr);
    fclose(routeFile);
    return host;
}

#endif
