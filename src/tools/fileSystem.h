/**********************************************************************************
*   Copyright © 2018 H-ZeX. All Rights Reserved.
*
*   File Name:    fileSystem.h
*   Author:       H-ZeX
*   Create Time:  2018-08-20-00:46:38
*   Describe:
*
**********************************************************************************/
#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include "src/util/def.h"
#include "ls.h"
#include "src/util/utility.h"

using namespace std;

/**
 * MT-safe
 * should use absolute path to call these func
 * for that when run in ide, the base path is not very sure
 * then these func may cause fatal error
 */
class FileSystem {
  public:
    /**
     * all these func
     * @return whether success
     */
    static bool ls(const char *path, std::string &result);
    static bool delFile(const char *pathname);
    static bool delDir(const char *pathname);
    static bool mkDir(const char *pathname);
    /**
     * @return (isSuccess, isDir);
     */
    static PBB isDir(const char *path);
    // this version is for the call outside of class
    static bool isPathAbsolute(const string& path);
    static bool isExistsAndReadable(const char *path);

  private:
    static bool isAbsolute(const string &path);
    static bool del(const char *pathname, bool isDir);
};
#endif
