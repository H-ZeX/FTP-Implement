/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    ls.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-10-17:53:28
*   Describe:
*
**********************************************************************************/
#include "List.h"

map<int, const char *const> List::monName{{0, "Jan"},  {1, "Feb"},  {2, "Mar"},  {3, "Apr"},
                                          {4, "May"},  {5, "June"}, {6, "July"}, {7, "Aug"},
                                          {8, "Sept"}, {9, "Oct"}, {10, "Nov"}, {11, "Dec"}};

int List::ls(const char *const path, std::string &result) {
    char errnoBuf[ERRNO_BUF_SIZE];
    struct stat statbuf;
    if (lstat(path, &statbuf) < 0) {
        result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
        return STAT_FAIL;
    }
    if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
        return lsDir(path, result);
    } else {
        return lsNotDir(path, result);
    }
}

int List::lsNotDir(const char *const path, std::string &result) {
    result.clear();
    struct stat statbuf;
    char errnoBuf[ERRNO_BUF_SIZE];
    if (lstat(path, &statbuf) < 0) {
        result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
        return STAT_FAIL;
    }
    // this bufSize also use to print the file size in decimal
    // for that, 32bit decimal is very large
    int bufSize = std::max<int>(MAXUSERNAMELEN, MAXGROUPNAMELEN);
    char printBuf[bufSize + 1];
    formatMode(statbuf.st_mode, result);
    result += " ";
    if (snprintf(printBuf, bufSize + 1, "%s", uid2username(statbuf.st_uid).c_str()) > bufSize) {
        bug("List::lsNotDir username too long", false);
    }
    result += printBuf;
    result += " ";
    if (snprintf(printBuf, bufSize + 1, "%s", gid2groupname(statbuf.st_gid).c_str()) > bufSize) {
        bug("List::lsNotDir groupname too long", false);
    }
    result += printBuf;
    result += " ";
    if (snprintf(printBuf, bufSize + 1, "%llu", (unsigned long long)statbuf.st_size) > bufSize) {
        bug("List::lsNotDir file size too large", false);
    }
    result += printBuf;
    result += " ";
    formatTime(&statbuf.st_atime, result);
    result += " ";
    std::string tmp = std::string(path);
    result += tmp.substr(tmp.rfind('/') + 1);
    result += END_OF_LINE;
    return SUCCESS;
}

int List::lsDir(const char *const path, std::string &result) {
    result.clear();
    // if pathname is too long, it'll override other stack variable
    // althrough stat func will return error and set errno == ENAMETOOLONG
    // I'll still check the length of pathname for that I don't know when
    // will stat find this error
    char pathname[PATH_MAX + 10], errnoBuf[ERRNO_BUF_SIZE];
    struct stat statbuf;
    DIR *streamp;
    off_t maxSize = 0, maxNameLen = 0;
    streamp = opendir(path);
    if (!streamp) {
        result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
        return OPEN_DIR_FAIL;
    }
    // every time call readdir, should set errno=0, in order to check whether there are error
    // occur
    errno = 0;
    for (dirent *dep = readdir(streamp);; errno = 0, dep = readdir(streamp)) {
        if (!dep && errno) {
            result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
            return READDIR_FAIL;
        } else if (!dep) {
            break;
        }
        mereString(pathname, (const char *const[]){path, "/", dep->d_name, 0}, PATH_MAX);
        // don't know whether stat is MT-safe, however, stat only need to read the shared data
        // structure, if the data structure isn't change when stat is call, **I believe** stat
        // is MT-safe. Also, vsftpd user stat without lock
        if (lstat(pathname, &statbuf) < 0) {
            result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
            return STAT_FAIL;
        }
        int t = uid2username(statbuf.st_uid).length();
        maxNameLen = std::max<off_t>(t, maxNameLen);
        t = gid2groupname(statbuf.st_gid).length();
        maxNameLen = std::max<off_t>(t, maxNameLen);
        maxSize = std::max<off_t>(maxSize, statbuf.st_size);
    }
    if (closedir(streamp) < 0) {
        bug("List::lsDir close dir error");
    }
    streamp = opendir(path);
    if (!streamp) {
        result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
        return OPEN_DIR_FAIL;
    }
    char fmtName[20], fmtSize[20], printBuf[maxNameLen + 10];
    if (snprintf(fmtName, 20, "%%%llus", (ull_t)maxNameLen) > 19) {
        bug("List::lsDir buffer for fmName too short", false);
    }
    if (snprintf(fmtSize, 20, "%%%dllu", numLen<ull_t>((ull_t)maxSize, 10)) > 19) {
        bug("List::lsDir buffer for fmtSize to short", false);
    }
    errno = 0;
    for (dirent *dep = readdir(streamp);; errno = 0, dep = readdir(streamp)) {
        if (!dep && errno) {
            result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
            return READDIR_FAIL;
        } else if (!dep) {
            break;
        }
        mereString(pathname, (const char *const[]){path, "/", dep->d_name, 0}, PATH_MAX);
        if (lstat(pathname, &statbuf) < 0) {
            result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
            return STAT_FAIL;
        }
        formatMode(statbuf.st_mode, result);
        result += " ";
        if (snprintf(printBuf, maxNameLen + 1, fmtName, uid2username(statbuf.st_uid).c_str()) >
            maxNameLen) {
            bug("List::lsDir username too long", false);
        }
        result += printBuf;
        result += " ";
        if (snprintf(printBuf, maxNameLen + 1, fmtName, gid2groupname(statbuf.st_gid).c_str()) >
            maxNameLen) {
            bug("List::lsDir groupname too long", false);
        }
        result += printBuf;
        result += " ";
        int __k = numLen<ull_t>((ull_t)maxSize, 10);
        char printSizeBuf[__k + 1];
        if (snprintf(printSizeBuf, __k + 1, fmtSize, (ull_t)statbuf.st_size) > __k) {
            bug("List::lsDir file size too large", false);
        }
        result += printSizeBuf;
        result += " ";
        formatTime(&statbuf.st_atime, result);
        result += " ";
        result += dep->d_name;
        result += END_OF_LINE;
    }
    return SUCCESS;
}

std::string List::uid2username(uid_t id) {
    return id2name<uid_t, passwd, getUid_t, char * passwd::*>(id, getpwuid_r, &passwd::pw_name);
}

std::string List::gid2groupname(gid_t id) {
    return id2name<gid_t, group, getGid_t, char * group::*>(id, getgrgid_r, &group::gr_name);
}

void List::formatMode(int mode, std::string &result) {
    switch (mode & S_IFMT) {
    case S_IFBLK:
        result.push_back('b');
        break;
    case S_IFCHR:
        result.push_back('c');
        break;
    case S_IFDIR:
        result.push_back('d');
        break;
    case S_IFIFO:
        result.push_back('p');
        break;
    case S_IFLNK:
        result.push_back('l');
        break;
    case S_IFREG:
        result.push_back('-');
        break;
    case S_IFSOCK:
        result.push_back('s');
        break;
    default:
        result.push_back('?');
        break;
    }
    unsigned int mark = 0400; // octal
    for (int i = 0; i < 3; i++) {
        result.push_back("r-"[!(mode & mark)]);
        mark >>= 1;
        result.push_back("w-"[!(mode & mark)]);
        mark >>= 1;
        result.push_back("x-"[!(mode & mark)]);
        mark >>= 1;
    }
}
void List::formatTime(const time_t *timep, string &result) {
    struct tm time, *res;
    res = localtime_r(timep, &time);
    if (res == nullptr) {
        warningWithErrno("List::formatTime");
        result += std::to_string(0);
        return;
    }
    result += monName[res->tm_mon];
    result += " ";
    result += std::to_string(res->tm_mday);
    result += " ";
    result += std::to_string(res->tm_hour);
    result += ":";
    result += std::to_string(res->tm_min);
}
