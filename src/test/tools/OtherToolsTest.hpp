//
// Created by hzx on 19-3-13.
//

#ifndef FTP_SERVER_OTHER_TOOLS_TEST_HPP
#define FTP_SERVER_OTHER_TOOLS_TEST_HPP


#include <src/main/tools/ListFiles.hpp>
#include "src/main/tools/FileSystem.hpp"
#include "config.hpp"

class OtherToolsTest {
public:
    static void testListFiles() {
        string result;
        ListFiles::ls(TOOLS_TEST_LIST_DIR_1, result);
        cout << result << endl;
        result.clear();
        ListFiles::ls(TOOLS_TEST_LIST_DIR_2, result);
        cout << result << endl;
        result.clear();
        ListFiles::ls(TOOLS_TEST_LIST_FILE_1, result);
        cout << result << endl;
    }

    static void testFS() {
        assert(FileSystem::mkDir(TOOLS_TEST_MKDIR_1));
        assert(FileSystem::delFileOrDir(TOOLS_TEST_MKDIR_1));
        assert(FileSystem::mkDir(TOOLS_TEST_MKDIR_2));
        assert(FileSystem::delFileOrDir(TOOLS_TEST_MKDIR_2));

        assert(FileSystem::isExistsAndReadable(TOOLS_TEST_FILE_1));
        assert(!FileSystem::isPathAbsolute(TOOLS_TEST_DIR_1));
        PBB t = FileSystem::isDir(TOOLS_TEST_DIR_2);
        assert(t.first && t.second);
    }
};

#endif //FTP_SERVER_OTHER_TOOLS_TEST_HPP
