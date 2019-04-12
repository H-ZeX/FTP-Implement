#ifndef __NETWORK_MANAGER_H__
#define  __NETWORK_MANAGER_H__

#include "src/main/util/Def.hpp"
#include "src/main/util/NetUtility.hpp"
#include "src/main/util/Utility.hpp"

#include <cstdlib>
#include <ctime>
#include <src/main/util/Def.hpp>

using std::pair;
using std::string;

struct RecvCmdReturnValue {
    bool success{}, isEOF{};
    int cnt{};

    RecvCmdReturnValue(bool success, bool isEOF, int cnt)
            : success(success), isEOF(isEOF), cnt(cnt) {}

};

struct OpenDataListenReturnValue {
    bool success{};
    int port{};

    OpenDataListenReturnValue(bool success, int port)
            : success(success), port(port) {}
};

class NetworkManager {
public:
    int getCmdFd() {
        return this->cmdFd;
    }

    void setCmdFd(int fd) {
        if (fd < 0) {
            bug("NetworkManager::setCmdFD: the param is illegal");
        }
        this->cmdFd = fd;
    }

    OpenDataListenReturnValue openDataListen() {
        // Only one user will use this fd, so backLog set to 1.
        OpenListenFdReturnValue fd = openListenFd(FTP_DATA_CONNECTION_BACKLOG);
        this->dataListenFd = fd.success ? fd.listenFd : -1;
        return {fd.success, fd.port};
    }

    bool acceptDataConnect() {
        if (this->dataListenFd < 0) {
            bug("NetworkManager::acceptDataConnect: accept connection while this->dataListenFd<0");
        }
        this->dataFd = acceptConnect(this->dataListenFd);
        assert(this->dataFd < 0 || this->dataFd >= 3);
        return this->dataFd >= 0;
    }

    bool openDataConnection(const char *hostname, const char *port) {
        this->dataFd = openClientFd(hostname, port);
        assert(this->dataFd < 0 || this->dataFd >= 3);
        return this->dataFd >= 0;
    }

    bool closeDataListen() {
        if (this->dataListenFd < 0) {
            return false;
        }
        closeFileDescriptor(this->dataListenFd);
        this->dataListenFd = -1;
        return true;
    }

    bool closeDataConnect() {
        if (this->dataFd < 0) {
            return false;
        }
        closeFileDescriptor(this->dataFd);
        this->dataFd = -1;
        return true;
    }

    /**
     * @param buf contain the result, end with \0
     */
    RecvCmdReturnValue recvCmd(char *buf, size_t bufSize) {
        if (this->cmdFd < 0) {
            bug("NetworkManager::recvCmd: recvCmd while this->cmdFd<0");
        }
        ReadLineReturnValue ret = readLine(this->cmdFd, buf, bufSize, bufForCmdFd);
        if (!ret.success) {
            bool isEOF = consumeByteUntilEndOfLine(this->cmdFd, bufForCmdFd);
            return {false, isEOF, -1};
        } else {
            return {true, ret.isEOF, static_cast<int>(ret.recvCnt)};
        }
    }

    bool sendToCmdFd(const char *msg, size_t len) {
        if (this->cmdFd < 0) {
            bug("NetworkManager::sendToCmdFd: sendToCmdFd while this->cmdFd<0");
        }
        return writeAllData(this->cmdFd, msg, len);
    }

    bool sendToDataFd(const byte *data, size_t len) {
        if (this->dataFd < 0) {
            bug("NetworkManager::sendToDataFd: sendToDataFd while this->dataFd<0");
        }
        return writeAllData(this->dataFd, data, len);
    }

    bool sendLocalFile(const char *const path) {
        if (this->dataFd < 0) {
            bug("NetworkManager::sendLocalFile: sendLocalFile while this->dataFd<0");
        }
        int localFileFd = openWrapV1(path, O_RDONLY);
        if (localFileFd == -1) {
            return false;
        }
        byte buf[READ_BUF_SIZE + 10];
        ReadBuf readBuf{};
        while (true) {
            int t = readWithBuf(localFileFd, buf, READ_BUF_SIZE, readBuf);
            if (t < 0) {
                warningWithErrno("sendLocalFile readWithBuf failed", errno);
                closeFileDescriptor(localFileFd);
                return false;
            } else if (t == 0) {
                closeFileDescriptor(localFileFd);
                return true;
            } else {
                if (!writeAllData(this->dataFd, buf, static_cast<size_t>(t))) {
                    closeFileDescriptor(localFileFd);
                    return false;
                }
            }
        }
    }

    bool recvRemoteAndWriteLocalFile(const char *const path) {
        if (this->dataFd < 0) {
            bug("NetworkManager::recvRemoteAndWriteLocalFile recvRemoteAndWriteLocalFile  while this->dataFd<0");
        }
        int localFileFd = openWrapV2(path, O_CREAT | O_WRONLY | O_TRUNC,
                                     S_IWUSR | S_IRUSR | S_IWOTH
                                     | S_IROTH | S_IWGRP | S_IRGRP);
        if (localFileFd < 0) {
            return false;
        }
        byte buf[RECV_BUF_SIZE + 10];
        ReadBuf cache{};
        while (true) {
            int hadRead = readWithBuf(this->dataFd, buf, RECV_BUF_SIZE, cache);
            if (hadRead < 0) {
                closeFileDescriptor(localFileFd);
                return false;
            } else if (hadRead > 0) {
                if (writeAllData(localFileFd, buf, static_cast<size_t>(hadRead))) {
                    continue;
                } else {
                    closeFileDescriptor(localFileFd);
                    return false;
                }
            } else {
                closeFileDescriptor(localFileFd);
                return true;
            }
        }
    }

    ~NetworkManager() {
        if (this->dataListenFd >= 3) {
            this->closeDataListen();
        }
        if (this->dataFd >= 3) {
            this->closeDataConnect();
        }
    }

private:
    /*
     * The cmdFd is pass from outside,
     * so DON'T close it.
     * The dataFd is opened by this class,
     * so should close it.
     *
     * The dataFd and dataListenFd should be -1 is it is not valid.
     * This means when endOfSession, should set them to -1.
     */
    int cmdFd = -1;
    int dataFd = -1;
    int dataListenFd = -1;
    ReadBuf bufForCmdFd{};
};

#endif
