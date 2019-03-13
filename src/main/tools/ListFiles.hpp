#ifndef __LS_H__
#define __LS_H__

#include "src/main/util/Def.hpp"
#include "src/main/util/Utility.hpp"
#include <cctype>
#include <dirent.h>
#include <dirent.h>
#include <cerrno>
#include <fcntl.h>
#include <grp.h>
#include <iostream>
#include <linux/limits.h>
#include <map>
#include <pthread.h>
#include <pwd.h>
#include <semaphore.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Thread-Safety: Unknown
 */
class ListFiles {
private:
    const static int MAX_USERNAME_LEN = 1024;
    const static int MAX_GROUP_NAME_LEN = 1024;
public:
    static bool ls(const char *path, std::string &result) {
        struct stat statBuf{};
        if (!lstatWrap(path, statBuf)) {
            return false;
        }
        if (isDir(statBuf)) {
            return lsDir(path, result);
        } else {
            return lsNotDir(path, result);
        }
    }

private:
    /**
     * @param result this string will be clear before store result
     */
    static bool lsNotDir(const char *path, std::string &result) {
        result.clear();
        struct stat statBuf{};
        if (!lstatWrap(path, statBuf)) {
            return false;
        }
        // this bufSize also use to print the file size in decimal
        // for that, 32bit decimal is very large
        const size_t bufSize = std::max<size_t>(MAX_USERNAME_LEN,
                                                MAX_GROUP_NAME_LEN);
        char printBuf[bufSize + 1];
        formatMode(statBuf.st_mode, result);
        result += " ";
        if (snprintf(printBuf, (bufSize + 1), "%s",
                     uid2Username(statBuf.st_uid).c_str()) > bufSize) {
            bug("ListFiles::lsNotDir failed: username too long", true);
        }
        result += printBuf;
        result += " ";
        if (snprintf(printBuf, bufSize + 1, "%s", gid2GroupName(statBuf.st_gid).c_str()) > bufSize) {
            bug("ListFiles::lsNotDir groupName too long", false);
        }
        result += printBuf;
        result += " ";
        if (snprintf(printBuf, bufSize + 1, "%llu",
                     (unsigned long long) statBuf.st_size) > bufSize) {
            bug("ListFiles::lsNotDir file size too large", false);
        }
        result += printBuf;
        result += " ";
        formatTime(statBuf.st_atime, result);
        result += " ";
        std::string tmp = std::string(path);
        result += tmp.substr(tmp.rfind('/') + 1);
        result += END_OF_LINE;
        return true;
    }

    static bool lsDir(const string &path, std::string &result) {
        result.clear();
        char pathname[PATH_MAX + 10], errnoBuf[ERRNO_BUF_SIZE];
        struct stat statBuf{};
        DIR *stream = openDirWrap(path);
        off_t maxSize = 0, maxNameLen = 0;
        // Every time call readdir, should set errno=0,
        // in order to check whether there are error occur
        errno = 0;
        for (dirent *dep = readdir(stream);; errno = 0, dep = readdir(stream)) {
            if (!dep && errno) {
                result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
                return false;
            } else if (!dep) {
                break;
            }
            mereString(pathname, (const char *const[]) {path.c_str(), "/",
                                                        dep->d_name, nullptr},
                       PATH_MAX);
            if (!lstatWrap(path, statBuf)) {
                return false;
            }
            maxNameLen = std::max<off_t>(uid2Username(statBuf.st_uid).length(), maxNameLen);
            maxNameLen = std::max<off_t>(gid2GroupName(statBuf.st_gid).length(), maxNameLen);
            maxSize = std::max<off_t>(maxSize, statBuf.st_size);
        }
        // to make the bufSize larger enough
        // TODO find a better way
        maxSize *= 1000000;
        closeDirWrap(stream);

        stream = openDirWrap(path);
        if (!stream) {
            return false;
        }
        char fmtName[20], fmtSize[20], printBuf[maxNameLen + 10];
        if (snprintf(fmtName, 20, "%%%llus", (ull_t) maxNameLen) > 19) {
            bug("ListFiles::lsDir buffer for fmName too short", false);
        }
        if (snprintf(fmtSize, 20, "%%%dllu", numLen<ull_t>((ull_t) maxSize, 10)) > 19) {
            bug("ListFiles::lsDir buffer for fmtSize to short", false);
        }
        errno = 0;
        for (dirent *dep = readdir(stream);; errno = 0, dep = readdir(stream)) {
            if (dep == nullptr && errno) {
                result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
                return false;
            } else if (dep == nullptr) {
                break;
            }
            mereString(pathname, (const char *const[]) {path.c_str(), "/", dep->d_name, nullptr}, PATH_MAX);
            if (lstat(pathname, &statBuf) < 0) {
                result = strerror_r(errno, errnoBuf, ERRNO_BUF_SIZE);
                return false;
            }
            formatMode(statBuf.st_mode, result);
            result += " ";
            if (snprintf(printBuf, static_cast<size_t>(maxNameLen + 1),
                         fmtName, uid2Username(statBuf.st_uid).c_str())
                > maxNameLen) {
                bug("ListFiles::lsDir username too long", false);
            }
            result += printBuf;
            result += " ";
            if (snprintf(printBuf, static_cast<size_t>(maxNameLen + 1),
                         fmtName, gid2GroupName(statBuf.st_gid).c_str())
                > maxNameLen) {
                bug("ListFiles::lsDir groupName too long", false);
            }
            result += printBuf;
            result += " ";
            auto numLenOfMaxSize = static_cast<size_t>(numLen<size_t>(maxSize, 10));
            char printSizeBuf[numLenOfMaxSize + 1];
            if (snprintf(printSizeBuf, (numLenOfMaxSize + 1),
                         fmtSize, (ull_t) statBuf.st_size)
                > numLenOfMaxSize) {
                bug("ListFiles::lsDir file size too large", false);
            }
            result += printSizeBuf;
            result += " ";
            formatTime(statBuf.st_atime, result);
            result += " ";
            result += dep->d_name;
            result += END_OF_LINE;
        }
        return true;
    }

    static string gid2GroupName(gid_t id) {
        size_t bufSize = std::max<size_t>(static_cast<const size_t &>(Sysconf(_SC_GETGR_R_SIZE_MAX)),
                                          static_cast<const size_t &>(Sysconf(_SC_GETPW_R_SIZE_MAX)));
        bufSize = bufSize < 0 ? UNDETERMINED_LIMIT : bufSize;
        char *buf = new char[bufSize];
        group data{}, *result;
        while (getgrgid_r(id, &data, buf, bufSize, &result) != 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == ERANGE) {
                bufSize *= 2;
                delete[] buf;
                buf = new char[bufSize];
            } else {
                break;
            }
        }
        string groupName;
        if (result != nullptr) {
            groupName = result->gr_name;
        } else {
            groupName = "???";
        }
        delete[] buf;
        return groupName;
    }


    static std::string uid2Username(uid_t id) {
        size_t bufSize = std::max<size_t>(static_cast<const size_t &>(Sysconf(_SC_GETGR_R_SIZE_MAX)),
                                          static_cast<const size_t &>(Sysconf(_SC_GETPW_R_SIZE_MAX)));
        bufSize = bufSize < 0 ? UNDETERMINED_LIMIT : bufSize;
        char *buf = new char[bufSize];
        passwd data{}, *result;
        while (getpwuid_r(id, &data, buf, bufSize, &result) != 0) {
            if (errno == EINTR) {
                continue;
            } else if (errno == ERANGE) {
                bufSize *= 2;
                delete[]buf;
                buf = new char[bufSize];
            } else {
                break;
            }
        }
        string userName;
        if (result != nullptr) {
            userName = result->pw_name;
        } else {
            userName = "???";
        }
        delete[] buf;
        return userName;
    }

    /**
     * @param result this method will NOT clear the result string
     * before write to it
     */
    static void formatMode(int mode, std::string &result) {
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
        unsigned int mark = 0400;
        for (int i = 0; i < 3; i++) {
            result.push_back("r-"[!(mode & mark)]);
            mark >>= 1;
            result.push_back("w-"[!(mode & mark)]);
            mark >>= 1;
            result.push_back("x-"[!(mode & mark)]);
            mark >>= 1;
        }
    }

    /**
     * @param result this method will NOT clear the result string
     * before write to it
     */
    static void formatTime(const time_t &time, string &result) {
        struct tm timeBuf{};
        if (!localtimeWrap(time, timeBuf)) {
            return;
        }
        result += monName[timeBuf.tm_mon];
        result += " ";
        result += std::to_string(timeBuf.tm_mday);
        result += " ";
        result += std::to_string(timeBuf.tm_hour);
        result += ":";
        result += std::to_string(timeBuf.tm_min);
    }

private:
    static constexpr const char *monName[] = {"Jan", "Feb", "Mar",
                                              "Apr", "May", "June",
                                              "July", "Aug", "Sept",
                                              "Oct", "Nov", "Dec", nullptr};
};

#endif
