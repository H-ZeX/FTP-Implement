/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    networkSession.h
*   Author:       H-ZeX
*   Create Time:  2018-08-17-15:31:39
*   Describe:
*
**********************************************************************************/

#include "def.h"
#include "netUtility.h"
#include "utility.h"
#include <stdlib.h>
#include <time.h>
using std::pair;
using std::string;

class NetworkSession {
  public:
    /**
     * these functions below return true if success
     * the dataFd's listen queue is only 1
     */
    int getCmdFd();
    void setCmdFd(int fd);
    int getDataFd();
    PBI openDataListen();
    bool acceptDataConnect();
    bool openDataConnection(const char *const hostname, const char *port);
    bool closeDataListen();
    bool closeDataConnect();
    /**
     * @return ((isEND_OF_LINE, isEOF), size)
     * if isEOL == false, then will clear the stream to the EOL
     * (however, still return isEOL==false)
     * the buf will end with 0 and without END_OF_LINE
     */
    PPI recvCmd(char *buf, size_t bufSize = CMD_MAX_LEN);
    /**
     * @return whether write all data to the fd succeed
     */
    bool sendToCmdFd(const char *const msg, size_t len);
    bool sendToDataFd(const byte *data, size_t len);
    bool sendFile(const char *const path);
    bool recvAndWriteFile(const char *const path);
    NetworkSession();
    ~NetworkSession();

  private:
    /**
     * @return ((isSuccess, isEOF), cnt)
     */
    PPI recvFromDataFd(byte *data, size_t len);

    /**
     * the cmd connect should only be call by de destructors
     */
    bool closeCmdConnect();

  private:
    int cmdFd;
    int dataFd;
    int dataListenFd;
    ReadBuf bufForCmd;
};
