#ifndef __NET_UTILITY_H__
#define __NET_UTILITY_H__

#include "src/main/util/Utility.hpp"
#include "src/main/config/Config.hpp"

#include "Def.hpp"

#include <cerrno>
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
 */
int acceptConnect(int listenFd) {
    assert(listenFd >= 3);
    makeSureFdType(listenFd, S_IFSOCK);
    sockaddr_storage clientAddr{};
    socklen_t clientAddrSize = sizeof(clientAddr);
    do {
        int fd = accept(listenFd, (SA *) &clientAddr, &clientAddrSize);
        if (fd < 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EFAULT || errno == ENOTSOCK || errno == EOPNOTSUPP || errno == EPROTO) {
                bugWithErrno("acceptConnect accept failed", errno, true);
            } else {
                warningWithErrno("acceptConnect accept failed", errno);
                return -1;
            }
        } else {
            return fd;
        }
    } while (errno == EINTR);
    assert(false);
}

/**
 * @return if success, return file descriptor
 * @return -1 if failed
 *
 * MT-Safety: MT-Safe
 */
int openClientFd(const char *hostname, const char *port) {
    assert(hostname != nullptr && port != nullptr);
    int clientFd = 0, rc;
    addrinfo hints{}, *list;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    if ((rc = getaddrinfo(hostname, port, &hints, &list)) != 0) {
        if (rc == EAI_SYSTEM) {
            warningWithErrno("openClientFd getaddrinfo failed", errno);
        } else {
            string errMsg = "openClientFd getaddrinfo failed: " + string(gai_strerror(rc));
            warning(errMsg.c_str());
        }
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
 */
int openListenFd(const char *port, int backlog = DEFAULT_BACKLOG) {
    assert(port != nullptr && backlog > 0);
    addrinfo hints{}, *list;
    int listenFd = 0, opt = 1;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_flags |= AI_NUMERICSERV;
    // getaddrinfo is MT-safe env local
    int rc = 0;
    if ((rc = getaddrinfo(nullptr, port, &hints, &list)) != 0) {
        if (rc == EAI_SYSTEM) {
            warningWithErrno("openListenFd getaddrinfo failed", errno);
        } else {
            string errMsg = "openListenFd getaddrinfo failed: " + string(gai_strerror(rc));
            warning(errMsg.c_str());
        }
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
    if (listen(listenFd, backlog) < 0) {
        // TODO errno handler should be refined
        warningWithErrno("openListenFd(port, backlog) listen failed", errno);
        return -1;
    }
    return listenFd;
}

struct OpenListenFdReturnValue {
    bool success{};
    int listenFd{}, port{};

    OpenListenFdReturnValue(bool success, int listenFd, int port) :
            success(success), listenFd(listenFd), port(port) {}
};

/**
 * Open an listenFd without specify the port(the OS will find an available port).
 */
OpenListenFdReturnValue openListenFd(int backlog = DEFAULT_BACKLOG) {
    assert(backlog > 0);
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        if (errno == EACCES || errno == EAFNOSUPPORT
            || errno == EINVAL || errno == EPROTONOSUPPORT) {
            bugWithErrno("openListenFd(backlog) socket failed", errno, true);
        } else {
            warningWithErrno("openListenFd(backlog) socket failed", errno);
            return {false, -1, -1};
        }
    }
    int opt = 1;
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(int)) < 0) {
        bugWithErrno("openListenFd(backlog) setsockopt failed", errno, true);
    }
    if (listen(listenFd, backlog) < 0) {
        if (errno == EADDRINUSE) {
            warningWithErrno("openListenFd(backlog) listen failed", errno);
            closeFileDescriptor(listenFd);
            return {false, -1, -1};
        } else {
            bugWithErrno("openListenFd(backlog) listen failed", errno, true);
        }
    }
    sockaddr_in sin{};
    socklen_t len = sizeof(sin);
    if (getsockname(listenFd, (struct sockaddr *) &sin, &len) < 0) {
        if (errno == ENOBUFS) {
            closeFileDescriptor(listenFd);
            warningWithErrno("openListenFd(backlog) getsockname failed", errno);
            return {false, -1, -1};
        } else {
            bugWithErrno("openListenFd(backlog) getsockname failed", errno, true);
        }
    }
    char netInfo[1024];
    if (inet_ntop(AF_INET, &sin.sin_addr, netInfo, sizeof(netInfo) - 10) == nullptr) {
        bugWithErrno("openListenFd(backlog) inet_ntop failed", errno, true);
    }
    return {true, listenFd, ntohs(sin.sin_port)};
}

/**
 * @return the result
 * if failed, return empty string
 * MT-safe env local
 */
const string getThisMachineIp() {
    char line[1024]{}, *p{}, *c{}, *save{};
    FILE *routeFile = fopen("/proc/net/route", "r");
    while (fgets(line, sizeof(line) - 10, routeFile)) {
        p = strtok_r(line, " \t", &save);
        c = strtok_r(nullptr, " \t", &save);
        if (p != nullptr && c != nullptr) {
            if (strncmp(c, "00000000", 8) == 0) {
                break;
            }
        }
    }
    int fm = AF_INET, family{};
    ifaddrs *addr{};
    char host[NI_MAXHOST + 10]{};
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
                            sizeof(sockaddr_in),
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
