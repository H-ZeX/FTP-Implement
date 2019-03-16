#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include "src/main/util/Def.hpp"
#include "ListFiles.hpp"
#include "src/main/util/Utility.hpp"

using namespace std;

/**
 * Thread-Safety: Unknown
 */
class FileSystem {
public:
    static bool ls(const char *path, std::string &result) {
        return isPathAbsolute(path) && (ListFiles::ls(path, result));
    }

    static bool delFileOrDir(const char *pathname) {
        return isPathAbsolute(pathname) && removeFile(pathname);
    }

    static bool mkDir(const char *pathname) {
        return isPathAbsolute(pathname) && mkdirWrap(pathname, S_IWUSR | S_IRUSR | S_IRGRP
                                                               | S_IWGRP | S_IROTH | S_IWOTH);
    }

    /**
     * @return (isSuccess, isDir);
     */
    static PBB isDir(const char *path) {
        if (!isPathAbsolute(path)) {
            return PBB(false, false);
        }
        struct stat statBuf{};
        // here should use stat but not lstat
        if (!statWrap(path, statBuf)) {
            return PBB(false, false);
        }
        return PBB(true, ((statBuf.st_mode & S_IFMT) == S_IFDIR));
    }

    static bool isExistsAndReadable(const char *path) {
        return isPathAbsolute(path) && euidAccessWrap(path, R_OK);
    }

    static bool isPathAbsolute(const string &path) {
        return path[0] == '/';
    }
};

#endif
