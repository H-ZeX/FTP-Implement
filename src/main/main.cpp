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

int main(int argc, char **argv) {
    int port;
    if (argc <= 1) {
        cerr << "usage: nc <port>, if port is not specified, user 8001" << endl;
        port = 8001;
    } else {
        port = std::stoi(argv[1]);
    }
    FTP *ftp = FTP::getInstance(port);
    ftp->startAndRun();
}
