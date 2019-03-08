/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    main.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-10-17:52:58
*   Describe:
*
**********************************************************************************/

#include "src/ftp/FTP.h"

int main() {
    FTP ftp;
    int port = 8088;

    ftp.start(port);
}
