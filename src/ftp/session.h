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

#include "src/util/def.h"
#include "src/tools/fileSystem.h"
#include "src/tools/ls.h"
#include "src/util/netUtility.h"
#include "networkSession.h"
#include "prelogin.h"
#include "src/util/utility.h"
#include <map>
#include <string>
using std::string;
using std::map;

/**
 * there will be only one thread to handler a session,
 * this means that when the data connection is busy,
 * the cmd connection will not response
 */
class Session {
  public:
    /**
     * @param callback_1 is callback when end of a cmd
     * @param callback_2 is callback when end of session
     */
    Session(int cmdFd, void (*callback_1)(void *), void (*callback_2)(void *), const string& thisMachineIP);
    void handle();
    int getCmdFd();

  private:
    void cdup(int paramIndex);
    void cwd(int paramIndex);
    void dele(int paramIndex);
    void help(int paramIndex);
    void list(int paramIndex);
    void mkd(int paramIndex);
    void mode(int paramIndex);
    void pass(int paramIndex);
    void pasv(int paramIndex);
    void port(int paramIndex);
    void pwd(int paramIndex);
    void quit(int paramIndex);
    void rest(int paramIndex);
    void retr(int paramIndex);
    void rmd(int paramIndex);
    void stat(int paramIndex);
    void stor(int paramIndex);
    void stru(int paramIndex);
    void syst(int paramIndex);
    void type(int paramIndex);
    void user(int paramIndex);

    void invalid();
    void haventLogin();
    void init();

  private:
    bool parsePortCmd(string param, string &ip, int &port);
    string makeAbsolutePath(const string &path);
    /**
     * the result store in the ip string
     */
    void formatPasvReply(std::string &ip, int port);
    void endHandler();
    void close();

  private:
    /**
     * the pasv, port will set these two var
     * the list, retr, stor will check and reset these two var
     *
     * for other cmd, these two var should not be used
     */
    
    string lastCmd;
    bool isLastCmdSuccess;
  public:
    char cmdBuf[CMD_MAX_LEN];
  private:
    const string selfIp;
    string currentPath; // absolute path, must end with '/'
    UserInfo userInfo;
    NetworkSession net;
    FileSystem fs;

    void (*callbackOnSessionEnd)(void *);
    void (*callbackOnCmdEnd)(void *);

  private:
    const std::map<string, void (Session::*)(int)> handlerMap{
        {"CDUP", &Session::cdup}, {"CWD", &Session::cwd},   {"DELE", &Session::dele},
        {"HELP", &Session::help}, {"LIST", &Session::list}, {"MKD", &Session::mkd},
        {"MODE", &Session::mode}, {"PASS", &Session::pass}, {"PASV", &Session::pasv},
        {"PORT", &Session::port}, {"PWD", &Session::pwd},   {"QUIT", &Session::quit},
        {"REST", &Session::rest}, {"RETR", &Session::retr}, {"RMD", &Session::rmd},
        {"STAT", &Session::stat}, {"STOR", &Session::stor}, {"STRU", &Session::stru},
        {"SYST", &Session::syst}, {"TYPE", &Session::type}, {"USER", &Session::user}};
};
#endif
