/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    ls.h
*   Author:       H-ZeX
*   Create Time:  2018-08-14-22:37:58
*   Describe:
*
**********************************************************************************/

#ifndef __LS_H__
#define __LS_H__

#include "def.h"
#include "utility.h"
#include <ctype.h>
#include <dirent.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <iostream>
#include <linux/limits.h>
#include <map>
#include <pthread.h>
#include <pwd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXUSERNAMELEN 32
#define MAXGROUPNAMELEN 32

/**
 * MT-safe
 */
class List {
    /**
     * return 1 if can't opendir
     * return 2 if error occurs when call readdir
     * return 0 if succeed
     */
  public:
    static int ls(const char *const path, std::string &result);

  private:
    static int lsNotDir(const char *const path, std::string &result);
    static int lsDir(const char *const path, std::string &result);
    static std::string uid2username(uid_t id);
    static std::string gid2groupname(gid_t id);
    static void formatMode(int mode, std::string &result);
    static void formatTime(const time_t *timep, string &result);

  private:
    /**
     * @return ??? can't find the name or errnr occur return ??? or the name is ???
     * this func only suitable for get username or get groupname
     */
    template <typename TId, typename TData, typename TFun, typename TCMemberPtr>
    static std::string id2name(TId id, TFun handler, TCMemberPtr ptr) {
        long bufSize = std::max<long>(Sysconf(_SC_GETGR_R_SIZE_MAX), Sysconf(_SC_GETPW_R_SIZE_MAX));
        // if bufSize<0, then _SC_GETPW_R_SIZE_MAX or _SC_GETGR_R_SIZE_MAX is undeterminate
        bufSize = bufSize + (bufSize < 0) * (UNDETERMINATE_LIMIT - bufSize);
        char buf[bufSize];
        TData data, *res;
        int t;
        errno_t ppp[] = {EINTR, ENFILE, ENOMEM, EMFILE, 0};
        errnoRetryV_2(handler(id, &data, buf, bufSize, &res), ppp, "List::id2name failed", t);
        if (res != NULL) {
            return res->*ptr;
        } else {
            return "???";
        }
    }

  private:
    typedef int(getGid_t)(gid_t, group *, char *, size_t, group **);
    typedef int(getUid_t)(uid_t, passwd *, char *, size_t, passwd **);
    static map<int, const char *const> monName;
};
#endif