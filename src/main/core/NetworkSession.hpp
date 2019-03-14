#include "src/main/util/Def.hpp"
#include "src/main/util/NetUtility.hpp"
#include "src/main/util/Utility.hpp"
#include <cstdlib>
#include <ctime>
#include <src/main/util/Def.hpp>

using std::pair;
using std::string;

class NetworkSession {
public:
    int getCmdFd() {
        return this->cmdFd;
    }

    void setCmdFd(int fd) {
        if (fd < 0) {
            bug("NetworkSession::setCmdFD: the param is illegal");
        }
        fprintf(stderr, "NetworkSession::setCmdFd: %d\n", fd);
        this->cmdFd = fd;
    }

    PBI openDataListen() {
        // Only one user will use this fd, so backLog set to 1.
        PII fd = openListenFd(1);
        this->dataListenFd = fd.first;
        return PBI(fd.first > 0, fd.second);
    }

    bool acceptDataConnect() {
        this->dataFd = acceptConnect(this->dataListenFd);
        return this->dataFd >= 0;
    }

    bool openDataConnection(const char *hostname, const char *port) {
        this->dataFd = openClientFd(hostname, port);
        return this->dataFd >= 0;
    }

    bool closeDataListen() {
        return closeFileDescriptor(this->dataListenFd);
    }

    bool closeDataConnect() {
        return closeFileDescriptor(this->dataFd);
    }
    /**
     * @return ((isEND_OF_LINE, isEOF), size)
     * if isEOL == false, then will clear the stream to the EOL
     * (however, still return isEOL==false)
     * the buf will end with 0 and without END_OF_LINE
     */
    PPI recvCmd(char *buf, size_t bufSize = CMD_MAX_LEN) {
        PPI p = readLine(this->cmdFd, buf, bufSize, bufForCmd);
        if (!p.first.first) {
            bool t = consumeByteUntilEndOfLine(this->cmdFd, bufForCmd);
            return PPI(PBB(false, t), p.second);
        } else {
            return p;
        }
    }

    /**
     * @return whether write all data to the fd succeed
     */
    bool sendToCmdFd(const char *msg, size_t len) {
        return writeAllData(this->cmdFd, (const byte *) msg, len);
    }

    bool sendToDataFd(const byte *data, size_t len) {
        return writeAllData(this->dataFd, data, len);
    }

    bool sendFile(const char *path) {
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

    bool recvAndWriteFile(const char *path) {
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

    NetworkSession() {
        this->cmdFd = -1;
        this->dataFd = -1;
        this->dataListenFd = -1;
    }

    ~NetworkSession() {
        this->closeDataListen();
        this->closeDataConnect();
    }

private:
    /**
     * @return ((isSuccess, isEOF), cnt)
     */
    PPI recvFromDataFd(byte *data, size_t len) {
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

private:
    int cmdFd;
    int dataFd;
    int dataListenFd;
    ReadBuf bufForCmd;
};
