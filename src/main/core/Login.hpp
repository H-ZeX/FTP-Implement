#ifndef __LOGIN_H__
#define __LOGIN_H__

#include "src/main/util/Utility.hpp"
#include <src/main/util/Def.hpp>

#include <crypt.h>
#include <shadow.h>
#include <string>

using std::string;

/**
 * @note
 * Will write the UserInfo's isValid, uid, gid, username, homeDir
 */
UserInfo login(const char *username,
               const char *pwd,
               const char *ip = "",
               const char *port = "") {
    /*
     * according to manual of crypt's glibc note, The size of this string is fixed:
     * MD5:22 characters, SHA-256: 43 characters, SHA-512: 86 characters
     * the $6$salt$encrypted is an SHA-512 encoded one, "salt" stands for the up to 16  characters
     */
    const size_t encryptKeyMaxLen = 256;
    spwd buf{}, *bufPtr = nullptr;
    size_t bufSize = encryptKeyMaxLen;
    char *spwdBuf = new char[bufSize];
    /*
     * The getspnam_r's manual is not clear,
     * I don't know whether the errno or the return value contain the error number.
     * So I reset the errno and check both of them.
     */
    int ret;
    errno = 0;
    while ((ret = getspnam_r(username, &buf, spwdBuf, bufSize - 10, &bufPtr)) != 0) {
        ret = ret == -1 ? errno : ret;
        if (ret == EACCES) {
            bugWithErrno("login getspnam_r failed", ret, true);
        } else if (ret == ERANGE) {
            delete[] spwdBuf;
            bufSize *= 2;
            spwdBuf = new char[bufSize];
            errno = 0;
        }
    }
    if (bufPtr == nullptr) {
        myLog((string("From ") + ip + ": " + username + " not found").c_str());
        delete[] spwdBuf;
        return UserInfo(false, 0, 0, ip, port, username, "");
    }
    struct crypt_data data{};
    data.initialized = 0;
    char *sp{};
    if ((sp = crypt_r(pwd, bufPtr->sp_pwdp, &data)) == nullptr) {
        if (errno == EINVAL || errno == EPERM) {
            bugWithErrno("login crypt_r failed", errno, true);
        } else if (errno == ENOSYS) {
            warningWithErrno("This OS DON'T support crypt function", errno);
            exit(0);
        } else {
            assert(false);
        }
    } else {
        // TODO this strncmp's char number limit is too large, which may have buffer overflow bug
        if (strncmp(sp, bufPtr->sp_pwdp, encryptKeyMaxLen) == 0) {
            myLog((string("From ") + ip + ": " + string(username) + " login success").c_str());
            UserInfo info = getUidGidHomeDir(username);
            info.cmdIp = ip;
            info.cmdPort = port;
            delete[] spwdBuf;
            return info;
        } else {
            myLog((string("From ") + ip + ": " + string(username) + " login failed").c_str());
            delete[] spwdBuf;
            return UserInfo(false, 0, 0, ip, port, username, "");
        }
    }
    assert(false);
}

#endif
