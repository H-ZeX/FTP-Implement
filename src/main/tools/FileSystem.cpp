/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    fileSystem.cpp
*   Author:       H-ZeX
*   Create Time:  2018-08-20-00:46:32
*   Describe:
*
**********************************************************************************/

#include "FileSystem.h"

bool FileSystem::ls(const char *const pathname, std::string &result) {
    return isAbsolute(pathname) && (ListFiles::ls(pathname, result) == 0);
}
bool FileSystem::delFile(const char *const pathname) {
    return isAbsolute(pathname) && del(pathname, false);
}
bool FileSystem::delDir(const char *const pathname) {
    return isAbsolute(pathname) && del(pathname, true);
}
bool FileSystem::mkDir(const char *const pathname) {
    if (!isAbsolute(pathname)) {
        return false;
    }
    if (mkdir(pathname, S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) {
        warningWithErrno("FileSystem::mkDir mkdir");
        return false;
    } else {
        return true;
    }
}
PBB FileSystem::isDir(const char *const pathname) {
    if (!isAbsolute(pathname)) {
        return PBB(false, false);
    }
    struct stat sb{};
    // here should use stat but not lstat
    if (::stat(pathname, &sb) < 0) {
        warningWithErrno((string("FileSystem::isDir stat ") + pathname).c_str());
        return PBB(false, false);
    }
    return PBB(true, ((sb.st_mode & S_IFMT) == S_IFDIR));
}
bool FileSystem::del(const char *const pathname, bool isDir) {
    if (!isAbsolute(pathname)) {
        return false;
    }
    PBB t = FileSystem::isDir(pathname);
    if (!t.first) {
        return false;
    } else {
        if (t.second && !isDir) {
            warning("Delete failed, this is directory");
            return false;
        } else if (!t.second && isDir) {
            warning("Delete failed, this is not directory");
            return false;
        }
    }
    if (remove(pathname) < 0) {
        warningWithErrno("Filesystem::del remove");
        return false;
    } else {
        return true;
    }
}
bool FileSystem::isAbsolute(const string &path) {
    if (path[0] == '/') {
        return true;
    } else {
        bug("FileSystem Should use absolute path", false);
        return false;
    }
}
bool FileSystem::isExistsAndReadable(const char *const path) {
    return isAbsolute(path) && euidaccess(path, R_OK) == 0;
}
bool FileSystem::isPathAbsolute(const string &path) {
    return path[0] == '/';
}
