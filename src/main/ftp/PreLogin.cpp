/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    PreLogin.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-21-11:24:41
*   Describe:
*
**********************************************************************************/
#include "PreLogin.h"
#include <iostream>
using std::cerr;
using std::endl;

UserInfo preLogin(const char *const username, const char *const passwd, const char *const ip,
                  const char *const port) {
    spwd spbuf{}, *spbufPtr;
    char spwdBuf[ENCRYPTED_KEY_MAX_LEN];
    int t;
    if ((t = getspnam_r(username, &spbuf, spwdBuf, ENCRYPTED_KEY_MAX_LEN, &spbufPtr)) != 0) {
        warningWithErrno("Session::login getspnam_r", t);
        return UserInfo(false, -1, -1, ip, port, username, "/home");
    } else if (spbufPtr == nullptr) {
        myLog((string("From ") + ip + ": " + std::string(username) + " not found").c_str());
        return UserInfo(false, -1, -1, ip, port, username, "/home");
    }
    struct crypt_data data{};
    data.initialized = 0;
    char *sp;
    if ((sp = crypt_r(passwd, spbufPtr->sp_pwdp, &data)) == nullptr) {
        warningWithErrno("Session::login crypt_r");
        return UserInfo(false, -1, -1, ip, port, username, "");
    } else {
        // TODO this strncmp's char number limit is too large, which may have buffer overflow bug
        if (strncmp(sp, spbufPtr->sp_pwdp, ENCRYPTED_KEY_MAX_LEN) == 0) {
            myLog((string("From ") + ip + ", " + string(username) + " login successed").c_str());
            auto gp = getUidGidHomeDir(username);
            gp.cmdIp = ip;
            gp.cmdPort = port;
            cerr << "preLogin: " << gp.username << endl;
            return gp;
        } else {
            warning((string("From ") + ip + ": " + string(username) + " login failed").c_str());
            return UserInfo(false, -1, -1, ip, port, username, "");
        }
    }
}
