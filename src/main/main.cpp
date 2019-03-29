/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    main.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-10-17:52:58
*   Describe:
*
**********************************************************************************/

#include "src/main/core/FTP.hpp"

bool changeLimit() {
    bool ok = true;

    rlimit limit{RLIM_INFINITY, RLIM_INFINITY};
    ok = ok & setRLimitWrap(RLIMIT_STACK, limit);

    limit = {MAX_FD_CNT, MAX_FD_CNT};
    ok = ok & setRLimitWrap(RLIMIT_NOFILE, limit);

    limit = {RLIM_INFINITY, RLIM_INFINITY};
    ok = ok & setRLimitWrap(RLIMIT_AS, limit);

    limit = {RLIM_INFINITY, RLIM_INFINITY};
    ok = ok & setRLimitWrap(RLIMIT_CORE, limit);

    limit = {RLIM_INFINITY, RLIM_INFINITY};
    ok = ok & setRLimitWrap(RLIMIT_DATA, limit);

    return ok;
}

int main(int argc, char **argv) {
    if (getuid() != 0) {
        fprintf(stderr, "warning: This server should run with sudo\n");
        exit(0);
    } else if (!changeLimit()) {
        fprintf(stderr, "warning: change process's limit failed, this server may not support too many connection\r\n");
    }
    int port;
    if (argc <= 1) {
        cerr << "usage: FTPServer <port>, if port is not specified, use 8001" << endl;
        port = 8001;
    } else {
        port = std::stoi(argv[1]);
    }
    FTP *ftp = FTP::getInstance(port);
    ftp->startAndRun();
}
