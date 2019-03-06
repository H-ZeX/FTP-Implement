/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    networkSession.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-16-16:56:52
*   Describe:
*
**********************************************************************************/
#include "networkSession.h"

NetworkSession::NetworkSession() {
    this->cmdFd = -1;
    this->dataFd = -1;
    this->dataListenFd = -1;
}

int NetworkSession::getCmdFd() {
    return this->cmdFd;
}

void NetworkSession::setCmdFd(int fd) {
    if (fd < 0) {
        bug("NetworkSession::setCmdFD: the param is illegal");
    }
    fprintf(stderr, "NetworkSession::setCmdFd: %d\n", fd);
    this->cmdFd = fd;
}

int NetworkSession::getDataFd() {
    return (this->dataFd >= 0) ? dataFd : -1;
}

PBI NetworkSession::openDataListen() {
    srand(clock());
    int port;
    for (int i = 0; i < 10; i++) {
        port = rand() % (65536 - 1024) + 1024;
        int t = openListenfd(std::to_string(port).c_str(), 1);
        if (t >= 0) {
            this->dataListenFd = t;
            break;
        }
    }
    return PBI(this->dataListenFd >= 0, port);
}

bool NetworkSession::acceptDataConnect() {
    this->dataFd = acceptConnect(this->dataListenFd);
    return this->dataFd >= 0;
}

bool NetworkSession::openDataConnection(const char *const hostname, const char *port) {
    this->dataFd = openClientfd(hostname, port);
    return this->dataFd >= 0;
}

bool NetworkSession::closeCmdConnect() {
    fprintf(stderr, "NetworkSession::closeFileDescriptor cmd fd == %d\n", this->cmdFd);
    return closeFileDescriptor(this->cmdFd, "NetworkSession cmdFd");
}

bool NetworkSession::closeDataListen() {
    return closeFileDescriptor(this->dataListenFd, "NetworkSession data listen fd");
}

bool NetworkSession::closeDataConnect() {
    return closeFileDescriptor(this->dataFd, "NetworkSession data connect fd");
}

PPI NetworkSession::recvCmd(char *buf, size_t bufSize) {
    PPI p = recvline(this->cmdFd, buf, bufSize, bufForCmd);
    if (!p.first.first) {
        bool t = recvToEOL(this->cmdFd, bufForCmd);
        return PPI(PBB(false, t), p.second);
    } else {
        return p;
    }
}

bool NetworkSession::sendToCmdFd(const char *msg, size_t len) {
    return writeAllData(this->cmdFd, (const byte *) msg, len);
}

bool NetworkSession::sendToDataFd(const byte *data, size_t len) {
    return writeAllData(this->dataFd, data, len);
}

bool NetworkSession::sendFile(const char *const path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        warningWithErrno("NetworkSession::sendFile");
        return false;
    }
    byte buf[READ_BUF_SIZE];
    int t;
    while ((t = read(fd, buf, READ_BUF_SIZE)) > 0 || errno == EINTR) {
        if (t > 0 && !this->sendToDataFd(buf, t)) {
            closeFileDescriptor(fd);
            return false;
        }
    }
    if (t == 0) {
        return closeFileDescriptor(fd);
    } else {
        warningWithErrno("NetworkSession::sendFile");
        return false;
    }
}

PPI NetworkSession::recvFromDataFd(byte *data, size_t len) {
    int t = 0, cnt = 0;
    while ((len - cnt > 0) &&
           ((t = read(this->dataFd, data + cnt, len - cnt)) > 0 || errno == EINTR)) {
        cnt += (t != -1) * t;
    }
    if (t == -1) {
        warningWithErrno("NetworkSession::recvFromDataFd");
    }
    return PPI(PBB(t != -1, t == 0), cnt);
}

bool NetworkSession::recvAndWriteFile(const char *const path) {
    int fd = open(path, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR | S_IWOTH | S_IROTH | S_IWGRP | S_IRGRP);
    if (fd == -1) {
        warningWithErrno("NetworkSession::recvAndWriteFile");
        return false;
    }
    byte buf[RECV_BUF_SIZE];
    while (true) {
        PPI t = recvFromDataFd(buf, RECV_BUF_SIZE);
        if (!t.first.first) {
            closeFileDescriptor(fd);
            return false;
        } else {
            if (t.second > 0 && !writeAllData(fd, buf, t.second)) {
                closeFileDescriptor(fd);
                return false;
            }
        }
        if (t.first.second) {
            return closeFileDescriptor(fd);
        }
    }
}

NetworkSession::~NetworkSession() {
//    this->closeCmdConnect();
    this->closeDataListen();
    this->closeDataConnect();
}
