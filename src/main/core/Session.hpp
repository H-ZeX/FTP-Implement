/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    session.h
*   Author:       H-ZeX
*   Create Time:  2018-08-17-20:18:00
*   Describe:
*
**********************************************************************************/
#ifndef __SESSION_H__
#define __SESSION_H__

#include "src/main/util/Def.hpp"
#include "src/main/tools/FileSystem.hpp"
#include "src/main/tools/ListFiles.hpp"
#include "src/main/util/NetUtility.hpp"
#include "src/main/util/Utility.hpp"

#include "FTP.hpp"
#include "NetworkSession.hpp"
#include "Login.hpp"
#include "Session.cpp"

#include <map>
#include <string>

using std::string;
using std::map;

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
     * @param endOfCmdCallback is callback when end of a cmd.
     * @param endOfSessionCallback is callback when end of this session.
     */
    explicit Session(int cmdFd,
                     void (*endOfCmdCallback)(void *),
                     void (*endOfSessionCallback)(void *)) {
        this->callbackOnCmdEnd = endOfCmdCallback;
        this->callbackOnSessionEnd = endOfSessionCallback;
        this->net.setCmdFd(cmdFd);
    }


    void handle() {
        // usleep(10000);
        /*
        const char *msg;
        if (userInfo.isValid == true && setThreadUidAndGid(userInfo.uid, userInfo.gid) == false) {
            msg = "400 Please loggin" END_OF_LINE;
            this->net.sendToCmdFd(msg, strlen(msg));
            this->close();
            return;
        }
        */
        fprintf(stderr, "Hander begin recvCmd\n");
        PPI p = this->net.recvCmd(this->cmdBuf, CMD_MAX_LEN);
        fprintf(stderr, "after recvcmd %d %d %d\n", p.first.first, p.first.second, p.second);

        try {
            // if no EOL, should check whether EOF, if EOF, should close the session
            // or will receive broken pipe signal
            if (!p.first.first) {
                if (p.first.second) {
                    this->close();
                    return;
                } else {
                    invalid();
                    this->endHandler();
                    return;
                }
            }
            PSI t = parseCmd(this->cmdBuf);
            fprintf(stderr, "parse cmd %s\n", t.first.c_str());
            auto k = handlerMap.find(t.first);
            if (k == handlerMap.end()) {
                invalid();
            } else if (!userInfo.isValid && (t.first != "USER" && t.first != "PASS")) {
                haventLogin();
            } else {
                (this->*(k->second))(t.second);
            }
            if (p.first.second) {
                this->close();
                return;
            } else {
                endHandler();
            }
        } catch (exception &e) {
            warning(e.what());
            this->callbackOnSessionEnd(this);
            return;
        }
    }

    int getCmdFd() {
        return this->net.getCmdFd();
    }

private:
    void cdup(int paramIndex) {
        (void) paramIndex;
        size_t len = currentPath.length();
        size_t t;
        if (len > 1) {
            if (*currentPath.rbegin() != '/') {
                bug("Session::cdup: currentPath should end with /");
            }
            t = currentPath.rfind('/', len - 2);
            if (t == string::npos) {
                bug("Session::cdup: The currentPath must have at least one '/'");
            }
            for (int i = t + 1; i < len; i++) {
                currentPath.pop_back();
            }
        }
        const char *msg = "250 Directory successfully changed" END_OF_LINE;
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void cwd(int paramIndex) {
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        PBB d = FileSystem::isDir(path.c_str());
        const char *msg;
        if (d.first && d.second) {
            currentPath = path + '/';
            warningWithErrno(("Session::cwd currentPath: " + currentPath).c_str());
            msg = "250 Directory successfully changed" END_OF_LINE;
        } else {
            msg = "550 Failed to change directory." END_OF_LINE;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void dele(int paramIndex) {
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        PBB t = FileSystem::isDir(path.c_str());
        const char *msg;
        if (!t.first || t.second) {
            msg = "550 Delete operation failed." END_OF_LINE;
        } else {
            if (!FileSystem::delFile(path.c_str())) {
                msg = "550 Delete operation failed." END_OF_LINE;
            } else {
                msg = "250 Delete operation successful." END_OF_LINE;
            }
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void help(int paramIndex) {
        (void) paramIndex;
        const char *msg = "214-CDUP, CWD, DELE, HELP, LIST, MKD, MODE, NLST, NOOP, PASS, PASV, PORT,\n "
                          "PWD, QUIT, REST, RETR, RMD, RNFR, RNTO, STAT, STOR, STRU, SYST, TYPE, "
                          "USER\n214 Help Ok" END_OF_LINE;
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void list(int paramIndex) {
        const char *msg;
        bool success = false;
        if ((lastCmd != "PORT" && lastCmd != "PASV") || !isLastCmdSuccess) {
            msg = "425 Use PORT or PASV first." END_OF_LINE;
        } else if (lastCmd == "PASV" && !this->net.acceptDataConnect()) {
            msg = "425 Failed to establish connection." END_OF_LINE;
        } else {
            msg = "150 Here comes the directory listing." END_OF_LINE;
            success = true;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
        if (!success) {
            break;
        }
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        string res;
        if (FileSystem::ls(path.c_str(), res)) {
            if (!this->net.sendToDataFd(res.c_str(), res.length())) {
                msg = "425 Failed to establish connection." END_OF_LINE;
            } else {
                msg = "226 Directory send OK." END_OF_LINE;
            }
        } else {
            msg = "400 LIST Failed" END_OF_LINE;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
        lastCmd = "";
        isLastCmdSuccess = false;
        this->net.closeDataListen();
        this->net.closeDataConnect();
    }

    void mkd(int paramIndex) {
        string msg;
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        if (!FileSystem::mkDir(path.c_str())) {
            msg = "550 Create directory operation failed." END_OF_LINE;
        } else {
            msg = ("257 \"" + path + "\" created" + END_OF_LINE);
        }
        if (!this->net.sendToCmdFd(msg.c_str(), msg.length())) {
            this->close();
            return;
        }
    }

    void mode(int paramIndex) {
        const char *msg;
        if (this->cmdBuf[paramIndex] != 'S' && this->cmdBuf[paramIndex] != 's') {
            msg = "504 Bad MODE command." END_OF_LINE;
        } else {
            msg = "200 Mode set to S." END_OF_LINE;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void pass(int paramIndex) {
        const char *msg;
        if (userInfo.isValid) {
            msg = "230 Already logged in." END_OF_LINE;
            break;
        }
        string pass = string(this->cmdBuf).substr(paramIndex);
        userInfo = preLogin(userInfo.username.c_str(), pass.c_str());
        if (!userInfo.isValid) {
            msg = "530 Login incorrect." END_OF_LINE;
            usleep(LOGIN_INCORRECT_DELAY_SEC * 1000000);
        } else {
            msg = "230 Login successful." END_OF_LINE;
            init();
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void pasv(int paramIndex) {
        (void) paramIndex;
        if (lastCmd == "PORT" || lastCmd == "PASV") {
            this->net.closeDataConnect();
            this->net.closeDataListen();
        }
        PBI t = this->net.openDataListen();
        string msg;
        if (!t.first || selfIp.length() == 0) {
            msg = "400 Failed to Enter Passive Mode" END_OF_LINE;
            this->net.closeDataListen();
        } else {
            string ip = selfIp;
            formatPasvReply(ip, t.second);
            msg = ("227 Entering Passive Mode (" + ip + ")" + END_OF_LINE);
        }
        if (!this->net.sendToCmdFd(msg.c_str(), msg.length())) {
            this->close();
            return;
        }
        fprintf(stderr, "pasv: Send to cmd fd END\n");
        this->isLastCmdSuccess = !(!t.first || selfIp.length() == 0);
        this->lastCmd = "PASV";
    }

    void port(int paramIndex) {
        if (lastCmd == "PORT" || lastCmd == "PASV") {
            this->net.closeDataConnect();
            this->net.closeDataListen();
        }
        int port;
        string ip;
        const char *msg;
        if (!parsePortCmd(string(this->cmdBuf).substr(paramIndex), ip, port)) {
            msg = "500 Illegal PORT command." END_OF_LINE;
            this->isLastCmdSuccess = false;
        } else {
            if (!this->net.openDataConnection(ip.c_str(), std::to_string(port).c_str())) {
                msg = "400 faild to establish connect." END_OF_LINE;
                this->isLastCmdSuccess = false;
            } else {
                msg = "200 PORT command successful. Consider using PASV." END_OF_LINE;
                this->isLastCmdSuccess = true;
            }
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
        this->lastCmd = "PORT";
    }

    void pwd(int paramIndex) {
        (void) paramIndex;
        string msg = ("257 \"" + currentPath + "\" is the current directory" + END_OF_LINE);
        if (!this->net.sendToCmdFd(msg.c_str(), msg.length())) {
            this->close();
            return;
        }
    }

    void quit(int paramIndex) {
        (void) paramIndex;
        const char *msg = "221 Goodbye" END_OF_LINE;
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
        this->close();
    }

    void rest(int paramIndex) {
        (void) paramIndex;
        const char *msg = "350 Restart position accepted (0)." END_OF_LINE;
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void retr(int paramIndex) {
        string msg;
        bool success = false;
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        if ((lastCmd != "PASV" && lastCmd != "PORT") || !isLastCmdSuccess) {
            msg = "425 Use PORT or PASV first." END_OF_LINE;
        } else {
            PBB isd = FileSystem::isDir(path.c_str());
            fprintf(stderr, "retr: %s %d %d %d", path.c_str(), isd.first, isd.second,
                    FileSystem::isExistsAndReadable(path.c_str()));
            if (!isd.first || isd.second ||
                !(FileSystem::isExistsAndReadable(path.c_str()))) {
                msg = "550 Failed to open file." END_OF_LINE;
            } else {
                success = true;
                msg = ("150 Opening data connection for " + path + END_OF_LINE);
            }
        }
        if (!this->net.sendToCmdFd(msg.c_str(), msg.length())) {
            this->close();
            return;
        }
        if (!success) {
            break;
        }
        if (lastCmd == "PASV" && !this->net.acceptDataConnect()) {
            msg = "425 Failed to establish connection." END_OF_LINE;
        } else if (this->net.sendFile(path.c_str())) {
            msg = "226 Transfer complete." END_OF_LINE;
        } else {
            msg = "425 Failed to establish connection." END_OF_LINE;
        }
        if (!this->net.sendToCmdFd(msg.c_str(), msg.length())) {
            this->close();
            return;
        }
        lastCmd = "";
        isLastCmdSuccess = false;
        this->net.closeDataListen();
        this->net.closeDataConnect();
    }

    void rmd(int paramIndex) {
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        const char *msg;
        if (!FileSystem::del(path.c_str())) {
            msg = "550 Remove directory operation failed." END_OF_LINE;
        } else {
            msg = "250 Remove directory operation successful." END_OF_LINE;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
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
        if (!this->net.sendToCmdFd(msg.c_str(), msg.length())) {
            this->close();
            return;
        }
    }

    void stor(int paramIndex) {
        const char *msg;
        bool success = false;
        if ((lastCmd != "PORT" && lastCmd != "PASV") || !isLastCmdSuccess) {
            msg = "425 Use PORT or PASV first." END_OF_LINE;
        } else if (lastCmd == "PASV" && !this->net.acceptDataConnect()) {
            msg = "425 failed to establish connection" END_OF_LINE;
        } else {
            msg = "150 Ok to send data." END_OF_LINE;
            success = true;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
        if (!success) {
            break;
        }
        string path = makeAbsolutePath(string(this->cmdBuf).substr(paramIndex));
        if (!this->net.recvAndWriteFile(path.c_str())) {
            msg = "400 STOR Failed" END_OF_LINE;
        } else {
            msg = "226 Transfer complete" END_OF_LINE;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
        lastCmd = "";
        isLastCmdSuccess = false;
        this->net.closeDataListen();
        this->net.closeDataConnect();
    }

    void stru(int paramIndex) {
        const char *msg;
        if (this->cmdBuf[paramIndex] != 'F' && this->cmdBuf[paramIndex] != 'f') {
            msg = "504 Bad STRU command." END_OF_LINE;
        } else {
            msg = "200 Structure set to F." END_OF_LINE;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void syst(int paramIndex) {
        (void) paramIndex;
        const char *msg = "215 UNIX" END_OF_LINE;
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void type(int paramIndex) {
        const char *msg = "504 Bad TYPE command." END_OF_LINE;
        if (this->cmdBuf[paramIndex] == 'A' || this->cmdBuf[paramIndex] == 'a') {
            msg = "200 Switching to ASCII mode." END_OF_LINE;
        } else if (this->cmdBuf[paramIndex] == 'I' || this->cmdBuf[paramIndex] == 'i') {
            msg = "200 Switching to Binary mode." END_OF_LINE;
        } else if ((this->cmdBuf[paramIndex] == 'L' || this->cmdBuf[paramIndex] == 'l') &&
                   this->cmdBuf[paramIndex + 2] == '8') {
            msg = "200 Switching to Binary mode." END_OF_LINE;
        }
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void user(int paramIndex) {
        const char *msg;
        string username = string(this->cmdBuf).substr(paramIndex);
        if (!userInfo.isValid) {
            msg = "331 Please specify the password" END_OF_LINE;
            userInfo.username = username;
        } else if (username == userInfo.username) {
            msg = "331 Any password will do." END_OF_LINE;
        } else {
            msg = "530 Can't change to another user." END_OF_LINE;
        }
        fprintf(stderr, "user cmd %s %s\n", username.c_str(), userInfo.username.c_str());
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void invalid() {
        const char *msg = "500 Unknown Command" END_OF_LINE;
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void haventLogin() {
        const char *msg = "530 Please login with USER and PASS" END_OF_LINE;
        if (!this->net.sendToCmdFd(msg, strlen(msg))) {
            this->close();
            return;
        }
    }

    void init() {
        this->currentPath = userInfo.homeDir;
        this->currentPath += "/";
    }

private:
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

private:
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
        int tmpi = 0;
        for (; i < param.length() && tmpi < 2; i++) {
            if (isdigit(param[i])) {
                tmp[tmpi] = tmp[tmpi] * 10 + param[i] - '0';
            } else if (param[i] == ',') {
                tmpi++;
            } else if (param[i] == ' ') {
                continue;
            } else {
                return false;
            }
        }
        if (tmpi > 1) {
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

    /**
     * the result store in the ip string
     */
    void formatPasvReply(std::string &ip, int port) {
        if (ip.length() == 0) {
            bug("Session::formatPasvReply pass invalid ip to here");
        }
        for (char &i : ip) {
            if (i == '.') {
                i = ',';
            }
        }
        ip = ip + "," + to_string(port / 256) + "," + to_string(port % 256);
    }

    void endHandler() {
        this->callbackOnCmdEnd(this);
    }


    /*
     * If called callbackOnSessionEnd, this session should NOT use anymore.
     * It is hard to use other ways except longjmp or exception to leave this session.
     * If use longjmp there may be some class in the stack which need to run destructors,
     * such as string. so longjmp will make the memory leakage.
     * The c++'s throw exception is similar to longjmp but will call the destructors in the stack.
     * So I use throw in the close func, and catch it in the suitable location.
     * This may seam a little special, but it is ok
     */
    void close() {
        throw EndOfSessionException();
    }

private:
    /*
     * The pasv, port will set these two var,
     * the list, retr, stor will check and reset these two var.
     * For any other cmd, these two var should NOT be used.
     */
    string lastCmd;
    bool isLastCmdSuccess = false;
public:
    char cmdBuf[CMD_MAX_LEN]{};
private:
    const string selfIp;
    // absolute path, MUST end with '/'
    string currentPath;
    UserInfo userInfo;
    NetworkSession net;

    void (*callbackOnSessionEnd)(void *);

    void (*callbackOnCmdEnd)(void *);

private:
    const std::map<string, void (Session::*)(int)> handlerMap{
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
            {"RMD",  &rmd},
            {"STAT", &stat},
            {"STOR", &stor},
            {"STRU", &stru},
            {"SYST", &syst},
            {"TYPE", &type},
            {"USER", &user}};
};

#endif
