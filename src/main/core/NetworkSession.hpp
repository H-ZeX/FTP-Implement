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

class NetworkSession {
public:
    int getCmdFd() {
        return this->cmdFd;
    }

    void setCmdFd(int fd) {
        if (fd < 0) {
            bug("NetworkSession::setCmdFD: the param is illegal");
        }
        this->cmdFd = fd;
    }

    OpenDataListenReturnValue openDataListen() {
        // Only one user will use this fd, so backLog set to 1.
        OpenListenFdReturnValue fd = openListenFd(1);
        this->dataListenFd = fd.success ? fd.listenFd : -1;
        return {fd.success, fd.port};
    }

    bool acceptDataConnect() {
        if (this->dataListenFd < 0) {
            bug("NetworkSession::acceptDataConnect: accept connection while this->dataListenFd<0");
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
            bug("NetworkSession::closeDataListen: closeDataListen while this->dataListenFd<0");
        }
        bool ret = closeFileDescriptor(this->dataListenFd);
        this->dataListenFd = -1;
        return ret;
    }

    bool closeDataConnect() {
        if (this->dataFd < 0) {
            bug("NetworkSession::closeDataConnect: closeDataListen while this->dataFd<0");
        }
        bool ret = closeFileDescriptor(this->dataFd);
        this->dataFd = -1;
        return ret;
    }

    RecvCmdReturnValue recvCmd(char *buf, size_t bufSize) {
        if (this->cmdFd < 0) {
            bug("NetworkSession::recvCmd: recvCmd while this->cmdFd<0");
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
            bug("NetworkSession::sendToCmdFd: sendToCmdFd while this->cmdFd<0");
        }
        return writeAllData(this->cmdFd, msg, len);
    }

    bool sendToDataFd(const byte *data, size_t len) {
        if (this->dataFd < 0) {
            bug("NetworkSession::sendToDataFd: sendToDataFd while this->dataFd<0");
        }
        return writeAllData(this->dataFd, data, len);
    }

    bool sendLocalFile(const char *const path) {
        if (this->dataFd < 0) {
            bug("NetworkSession::sendLocalFile: sendLocalFile while this->dataFd<0");
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
            bug("NetworkSession::recvRemoteAndWriteLocalFile recvRemoteAndWriteLocalFile  while this->dataFd<0");
        }
        int localFileFd = openWrapV2(path, O_CREAT | O_WRONLY,
                                     S_IWUSR | S_IRUSR | S_IWOTH
                                     | S_IROTH | S_IWGRP | S_IRGRP);
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
                return closeFileDescriptor(localFileFd);
            }
        }
    }

    ~NetworkSession() {
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
     * This means when close, should set them to -1.
     */
    int cmdFd = -1;
    int dataFd = -1;
    int dataListenFd = -1;
    ReadBuf bufForCmdFd{};
};
