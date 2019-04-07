#ifndef __SESSION_H__
#define __SESSION_H__

#include "src/main/util/Def.hpp"
#include "src/main/tools/FileSystem.hpp"
#include "src/main/tools/ListFiles.hpp"
#include "src/main/util/NetUtility.hpp"
#include "src/main/util/Utility.hpp"
#include "src/main/config/Config.hpp"

#include "NetworkManager.hpp"
#include "Login.hpp"
#include "Session.hpp"

#include <unordered_map>

using std::string;
using std::map;


class EndOfSessionException : public exception {
    const char *what() const noexcept override {
        return "EndOfSessionException";
    }
};


/**
 * @warning
 * This class is NOT MT-Safe!
 * @note
 * Only one thread can handler a session at any time,
 * this means that when the data connection is busy,
 * the cmd connection will not response.
 */
class Session {
public:
    /**
     * @param endOfCmdCallback callback when end of a cmd.
     * @param endOfSessionCallback callback when end of this session.
     * <br/>
     * @note
     * When end of session, will NOT call end of cmd callback.
     * <br/>
     * When called endOfCmdCallback, will NOT call endOfSessionCallback.
     */
    explicit Session(int cmdFd,
                     void (*endOfCmdCallback)(void *),
                     void (*endOfSessionCallback)(void *),
                     string thisMachineIp)
            : selfIp(thisMachineIp) {
        assert(cmdFd >= 3);
        this->callbackOnCmdEnd = endOfCmdCallback;
        this->callbackOnSessionEnd = endOfSessionCallback;
        this->networkManager.setCmdFd(cmdFd);
    }

    void handle() {
        RecvCmdReturnValue ret = networkManager.recvCmd(this->cmdBuf,
                                                        CMD_MAX_LEN);
        try {
            // if no EOL, should check whether EOF,
            // if EOF, should endOfSession the session
            // or will receive broken pipe signal
            //
            // When call endOfCmd, MUST make sure NOT not to call endOfSession.
            if (!ret.success) {
                if (ret.isEOF) {
                    this->endOfSession();
                    return;
                } else {
                    invalid();
                    this->endOfCmdHandler();
                    return;
                }
            }
            PSI cmd = parseCmd(this->cmdBuf);
            assert(cmd.second < CMD_MAX_LEN);
            myLog(("recvCmd: " + string(cmdBuf)).c_str());
            auto handler = handlerMap.find(cmd.first);
            if (handler == handlerMap.end()) {
                invalid();
            } else if (!userInfo.isValid
                       && cmd.first != "USER"
                       && cmd.first != "PASS") {
                hasNotLogin();
            } else {
                (this->*(handler->second))(cmd.second);
            }
            if (ret.isEOF) {
                this->endOfSession();
                return;
            } else {
                endOfCmdHandler();
            }
        } catch (exception &e) {
            if (this->callbackOnSessionEnd != nullptr) {
                this->callbackOnSessionEnd(this);
            }
            return;
        }
    }

    int getCmdFd() {
        return this->networkManager.getCmdFd();
    }

private:

    void cdup(int paramIndex) {
        (void) paramIndex;
        size_t len = currentPath.length();
        assert(len > 0);
        assert(currentPath[0] == '/' && currentPath[len - 1] == '/');
        // The first and last index of currentPath is `/`.
        // So if len == 1, then it is at root dir.
        if (len > 1) {
            int index = static_cast<int>(currentPath.rfind('/', len - 2));
            assert(index != string::npos);
            currentPath = currentPath.substr(0, (index + 1));
            assert(currentPath[0] == '/' && *currentPath.rbegin() == '/');
        }
        const char *msg = "250 Directory successfully changed" END_OF_LINE;
        sendToCmd(msg);
    }

    void cwd(int paramIndex) {
        string path = string(this->cmdBuf).substr(paramIndex);
        path = makeAbsolutePath(path);
        PBB isDir = FileSystem::isDir(path.c_str());
        const char *msg = nullptr;
        if (isDir.first && isDir.second) {
            currentPath = path + '/';
            msg = "250 Directory successfully changed" END_OF_LINE;
        } else {
            msg = "550 Failed to change directory." END_OF_LINE;
        }
        sendToCmd(msg);
    }

    void dele(int paramIndex) {
        string path = string(this->cmdBuf).substr(paramIndex);
        path = makeAbsolutePath(path);
        PBB isDir = FileSystem::isDir(path.c_str());
        const char *msg = nullptr;
        if (!isDir.first || isDir.second) {
            msg = "550 Delete operation failed." END_OF_LINE;
        } else {
            if (!FileSystem::delFileOrDir(path.c_str())) {
                msg = "550 Delete operation failed." END_OF_LINE;
            } else {
                msg = "250 Delete operation successful." END_OF_LINE;
            }
        }
        sendToCmd(msg);
    }

    void help(int paramIndex) {
        (void) paramIndex;
        const char *msg = "214-CDUP, CWD, DELE, HELP, LIST, MKD, "
                          "MODE, NLST, NOOP, PASS, PASV, PORT,\n "
                          "PWD, QUIT, REST, RETR, RMD, RNFR, RNTO, "
                          "STAT, STOR, STRU, SYST, TYPE, "
                          "USER\n214 Help Ok" END_OF_LINE;
        sendToCmd(msg);
    }

    void list(int paramIndex) {
        const char *msg = nullptr;
        bool success = false;
        if ((lastCmd != "PORT" && lastCmd != "PASV") || !isLastCmdSuccess) {
            msg = "425 Use PORT or PASV first." END_OF_LINE;
        } else if (lastCmd == "PASV" && !this->networkManager.acceptDataConnect()) {
            msg = "425 Failed to establish connection." END_OF_LINE;
        } else {
            msg = "150 Here comes the directory listing." END_OF_LINE;
            success = true;
        }
        if (!this->networkManager.sendToCmdFd(msg, strlen(msg))) {
            this->endOfSession();
            return;
        }
        if (!success) {
            closeDataConnectionOrListen();
            lastCmd = "";
            isLastCmdSuccess = false;
        } else {
            string path = string(this->cmdBuf).substr(paramIndex);
            path = makeAbsolutePath(path);
            string result;
            if (FileSystem::ls(path.c_str(), result)) {
                if (!this->networkManager.sendToDataFd(result.c_str(), result.length())) {
                    msg = "425 Failed to establish connection." END_OF_LINE;
                } else {
                    msg = "226 Directory send OK." END_OF_LINE;
                }
            } else {
                msg = "400 LIST Failed" END_OF_LINE;
            }
            closeDataConnectionOrListen();
            if (!this->networkManager.sendToCmdFd(msg, strlen(msg))) {
                this->endOfSession();
                return;
            }
            lastCmd = "";
            isLastCmdSuccess = false;
        }
    }

    void mkd(int paramIndex) {
        string msg;
        string path = string(this->cmdBuf).substr(paramIndex);
        path = makeAbsolutePath(path);
        if (!FileSystem::mkDir(path.c_str())) {
            msg = "550 Create directory operation failed." END_OF_LINE;
        } else {
            msg = ("257 \"" + path + "\" created" + END_OF_LINE);
        }
        sendToCmd(msg.c_str());
    }

    void mode(int paramIndex) {
        const char *msg = nullptr;
        const char c = this->cmdBuf[paramIndex];
        if (c != 'S' && c != 's') {
            msg = "504 Bad MODE command." END_OF_LINE;
        } else {
            msg = "200 Mode set to S." END_OF_LINE;
        }
        sendToCmd(msg);
    }

    void pass(int paramIndex) {
        const char *msg = nullptr;
        if (userInfo.isValid) {
            msg = "230 Already logged in." END_OF_LINE;
        } else {
            string pass = string(this->cmdBuf).substr(paramIndex);
            UserInfo tmp = login(userInfo.username.c_str(), pass.c_str());
            if (!tmp.isValid) {
                msg = "530 Login incorrect." END_OF_LINE;
                userInfo.isValid = false;
                usleep(LOGIN_FAIL_DELAY_SEC * 1000000);
            } else {
                msg = "230 Login successful." END_OF_LINE;
                userInfo.isValid = true;
                userInfo.uid = tmp.uid;
                userInfo.gid = tmp.gid;
                userInfo.homeDir = tmp.homeDir;
                this->currentPath = userInfo.homeDir + "/";
            }
        }
        sendToCmd(msg);
    }

    void pasv(int paramIndex) {
        (void) paramIndex;

        closeDataConnectionOrListen();
        this->lastCmd = "";
        this->isLastCmdSuccess = false;

        OpenDataListenReturnValue ret = this->networkManager.openDataListen();
        string msg;
        assert(selfIp.length() > 0);
        if (!ret.success) {
            msg = "400 Failed to Enter Passive Mode" END_OF_LINE;
            this->networkManager.closeDataListen();
        } else {
            string addr = formatPasvReply(selfIp, ret.port);
            msg = ("227 Entering Passive Mode (" + addr + ")" + END_OF_LINE);
            this->isLastCmdSuccess = true;
            this->lastCmd = "PASV";
        }
        sendToCmd(msg.c_str());
    }

    void port(int paramIndex) {
        closeDataConnectionOrListen();
        this->lastCmd = "";
        this->isLastCmdSuccess = false;

        int port = 0;
        string ip;
        const char *msg = nullptr;
        string param = string(this->cmdBuf).substr(paramIndex);
        if (!parsePortCmd(param, ip, port)) {
            msg = "500 Illegal PORT command." END_OF_LINE;
        } else if (!this->networkManager.openDataConnection(ip.c_str(), to_string(port).c_str())) {
            msg = "400 failed to establish connect." END_OF_LINE;
        } else {
            msg = "200 PORT command successful. Consider using PASV." END_OF_LINE;
            this->isLastCmdSuccess = true;
            this->lastCmd = "PORT";
        }
        sendToCmd(msg);
    }

    void pwd(int paramIndex) {
        (void) paramIndex;
        string msg = ("257 \"" + currentPath + "\" is the current directory" + END_OF_LINE);
        sendToCmd(msg.c_str());
    }

    void quit(int paramIndex) {
        (void) paramIndex;
        const char *msg = "221 Goodbye" END_OF_LINE;
        sendToCmd(msg);
        this->endOfSession();
    }

    void rest(int paramIndex) {
        (void) paramIndex;
        const char *msg = "350 Restart position accepted (0)." END_OF_LINE;
        sendToCmd(msg);
    }

    void retr(int paramIndex) {
        string msg;
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        bool success = false;
        if ((lastCmd != "PASV" && lastCmd != "PORT") || !isLastCmdSuccess) {
            msg = "425 Use PORT or PASV first." END_OF_LINE;
        } else {
            PBB isd = FileSystem::isDir(path.c_str());
            if (!isd.first || isd.second ||
                !FileSystem::isExistsAndReadable(path.c_str())) {
                msg = "550 Failed to open file." END_OF_LINE;
            } else {
                success = true;
                msg = ("150 Opening data connection for " + path + END_OF_LINE);
            }
        }
        if (!this->networkManager.sendToCmdFd(msg.c_str(), msg.length())) {
            this->endOfSession();
            return;
        }
        if (!success) {
            closeDataConnectionOrListen();
            lastCmd = "";
            isLastCmdSuccess = false;
            return;
        } else {
            if (lastCmd == "PASV" && !this->networkManager.acceptDataConnect()) {
                msg = "425 Failed to establish connection." END_OF_LINE;
            } else if (this->networkManager.sendLocalFile(path.c_str())) {
                msg = "226 Transfer complete." END_OF_LINE;
            } else {
                msg = "425 Failed to establish connection." END_OF_LINE;
            }
            closeDataConnectionOrListen();
            if (!this->networkManager.sendToCmdFd(msg.c_str(), msg.length())) {
                this->endOfSession();
                return;
            }
            lastCmd = "";
            isLastCmdSuccess = false;
        }
    }

    void rmd(int paramIndex) {
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        const char *msg = nullptr;
        if (!FileSystem::delFileOrDir(path.c_str())) {
            msg = "550 Remove directory operation failed." END_OF_LINE;
        } else {
            msg = "250 Remove directory operation successful." END_OF_LINE;
        }
        sendToCmd(msg);
    }

    void stat(int paramIndex) {
        string msg;
        if (this->cmdBuf[paramIndex] != 0) {
            string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
            string res;
            if (!FileSystem::ls(path.c_str(), res)) {
                msg = "213-Status follows:" END_OF_LINE "213 End of status" END_OF_LINE;
            } else {
                msg = ("213-Status follows" + string(END_OF_LINE) + res + "213 End of status" +
                       END_OF_LINE);
            }
        } else {
            msg = ("211-FTP server status:" + string(END_OF_LINE) + "     Connected to " +
                   userInfo.cmdIp + END_OF_LINE + "     Logged in as " + userInfo.username +
                   END_OF_LINE + "     " + PRODUCTION_NAME + " " + PRODUCTION_VERSION + END_OF_LINE +
                   "211 End of status" + END_OF_LINE);
        }
        sendToCmd(msg.c_str());
    }

    void stor(int paramIndex) {
        const char *msg = nullptr;
        bool success = false;
        if ((lastCmd != "PORT" && lastCmd != "PASV") || !isLastCmdSuccess) {
            msg = "425 Use PORT or PASV first." END_OF_LINE;
        } else if (lastCmd == "PASV" && !this->networkManager.acceptDataConnect()) {
            msg = "425 failed to establish connection" END_OF_LINE;
        } else {
            msg = "150 Ok to send data." END_OF_LINE;
            success = true;
        }
        if (!this->networkManager.sendToCmdFd(msg, strlen(msg))) {
            this->endOfSession();
            return;
        }
        if (!success) {
            closeDataConnectionOrListen();
            lastCmd = "";
            isLastCmdSuccess = false;
        } else {
            string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
            if (!this->networkManager.recvRemoteAndWriteLocalFile(path.c_str())) {
                msg = "400 STOR Failed" END_OF_LINE;
            } else {
                msg = "226 Transfer complete" END_OF_LINE;
            }
            closeDataConnectionOrListen();
            if (!this->networkManager.sendToCmdFd(msg, strlen(msg))) {
                this->endOfSession();
                return;
            }
            lastCmd = "";
            isLastCmdSuccess = false;
        }
    }

    void stru(int paramIndex) {
        const char *msg = nullptr;
        const char c = this->cmdBuf[paramIndex];
        if (c != 'F' && c != 'f') {
            msg = "504 Bad STRU command." END_OF_LINE;
        } else {
            msg = "200 Structure set to F." END_OF_LINE;
        }
        sendToCmd(msg);
    }

    void syst(int paramIndex) {
        (void) paramIndex;
        const char *msg = "215 UNIX" END_OF_LINE;
        sendToCmd(msg);
    }

    void type(int paramIndex) {
        const char *msg = "504 Bad TYPE command." END_OF_LINE;
        char c = this->cmdBuf[paramIndex];
        c = (c <= 'z' && c >= 'a') ? c : ('a' + (c - 'A'));
        if (c == 'a') {
            msg = "200 Switching to ASCII mode." END_OF_LINE;
        } else if (c == 'i' || c == 'l') {
            msg = "200 Switching to Binary mode." END_OF_LINE;
        }
        sendToCmd(msg);
    }

    void user(int paramIndex) {
        const char *msg = nullptr;
        string username = string(this->cmdBuf).substr(paramIndex);
        if (!userInfo.isValid) {
            msg = "331 Please specify the password" END_OF_LINE;
            userInfo.username = username;
        } else if (username == userInfo.username) {
            msg = "331 Any password will do." END_OF_LINE;
        } else {
            msg = "530 Can't change to another user." END_OF_LINE;
        }
        sendToCmd(msg);
    }

    void invalid() {
        const char *msg = "500 Unknown Command" END_OF_LINE;
        sendToCmd(msg);
    }

    void hasNotLogin() {
        const char *msg = "530 Please login with USER and PASS" END_OF_LINE;
        sendToCmd(msg);
    }

private:

    /**
     * @return (cmd, index of the end of cmd)
     */
    PSI parseCmd(const char *param) {
        int i = 0;
        while (param[i] && isspace(param[i])) {
            i++;
        }
        if (param[i] == 0) {
            return PSI("INVALID", 0);
        }
        string cmd;
        while (param[i] && isalpha(param[i])) {
            cmd.push_back(charUpper(param[i]));
            i++;
        }
        while (param[i] && isspace(param[i])) {
            i++;
        }
        return PSI(cmd, i);
    }

    bool parsePortCmd(string param, string &ip, int &port) {
        ip.clear(), port = 0;
        int cnt = 0, i = 0;
        for (; i < param.length() && cnt < 4; i++) {
            if (isdigit(param[i])) {
                ip.push_back(param[i]);
            } else if (param[i] == ',') {
                cnt++;
                ip.push_back('.');
            } else if (param[i] == ' ') {
                continue;
            } else {
                return false;
            }
        }
        if (cnt == 4) {
            ip.pop_back();
        } else {
            return false;
        }
        int tmp[2] = {0, 0};
        int tmpInd = 0;
        for (; i < param.length() && tmpInd < 2; i++) {
            if (isdigit(param[i])) {
                tmp[tmpInd] = tmp[tmpInd] * 10 + param[i] - '0';
            } else if (param[i] == ',') {
                tmpInd++;
            } else if (param[i] == ' ') {
                continue;
            } else {
                return false;
            }
        }
        if (tmpInd > 1) {
            return false;
        }
        port = tmp[0] * 256 + tmp[1];
        return true;
    }

    string makeAbsolutePath(const string &path) {
        if (!FileSystem::isPathAbsolute(path)) {
            return currentPath + path;
        } else {
            return path;
        }
    }

    string formatPasvReply(string ip, int port) {
        assert(ip.length() > 0);
        for (int j = 0; j < ip.length(); j++) {
            if (ip[j] == '.') {
                ip[j] = ',';
            }
        }
        string result = ip + ","
                        + to_string(port / 256) + ","
                        + to_string(port % 256);
        return result;
    }

    void endOfCmdHandler() {
        if (this->callbackOnCmdEnd != nullptr) {
            this->callbackOnCmdEnd(this);
        }
    }

    /*
     * If called callbackOnSessionEnd, this session should NOT use anymore.
     * It is hard to use other ways except longjmp or exception to leave this session.
     * There are some class in the stack which need to run destructors,
     * such as string. so longjmp will make the memory leakage.
     * The c++'s throw exception is similar to longjmp
     * but will call the destructors in the stack.
     * So I use throw in the endOfSession func, and catch it in the suitable location.
     * This may seam a little special, but it is ok
     */
    void endOfSession() {
        throw EndOfSessionException();
    }

    void sendToCmd(const char *const msg) {
        if (!this->networkManager.sendToCmdFd(msg, strlen(msg))) {
            this->endOfSession();
            return;
        }
    }

    void closeDataConnectionOrListen() {
        this->networkManager.closeDataListen();
        this->networkManager.closeDataConnect();
    }

private:
    /*
     * The pasv, port will set these two var,
     * the list, retr, stor will check and reset these two var.
     * For any other cmd, these two var should NOT be used.
     */
    string lastCmd{};
    bool isLastCmdSuccess = false;

    static const size_t CMD_MAX_LEN = 1024;
    char cmdBuf[CMD_MAX_LEN]{};

    // absolute path, MUST begin and end with '/'
    UserInfo userInfo{};

    void (*callbackOnCmdEnd)(void *){};

    void (*callbackOnSessionEnd)(void *){};

    NetworkManager networkManager{};
    const string selfIp{};
    string currentPath{};

    const std::unordered_map<string, void (Session::*)(int)> handlerMap{
            {"CDUP", &Session::cdup},
            {"CWD",  &Session::cwd},
            {"DELE", &Session::dele},
            {"HELP", &Session::help},
            {"LIST", &Session::list},
            {"MKD",  &Session::mkd},
            {"MODE", &Session::mode},
            {"PASS", &Session::pass},
            {"PASV", &Session::pasv},
            {"PORT", &Session::port},
            {"PWD",  &Session::pwd},
            {"QUIT", &Session::quit},
            {"REST", &Session::rest},
            {"RETR", &Session::retr},
            {"RMD",  &Session::rmd},
            {"STAT", &Session::stat},
            {"STOR", &Session::stor},
            {"STRU", &Session::stru},
            {"SYST", &Session::syst},
            {"TYPE", &Session::type},
            {"USER", &Session::user}};
};

#endif
