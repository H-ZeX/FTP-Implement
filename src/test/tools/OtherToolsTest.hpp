//
// Created by hzx on 19-3-13.
//

#ifndef FTP_SERVER_OTHER_TOOLS_TEST_HPP
#define FTP_SERVER_OTHER_TOOLS_TEST_HPP


#include <src/main/tools/ListFiles.hpp>
#include "src/main/tools/FileSystem.hpp"

class OtherToolsTest {
public:
    static void testListFiles() {
        string result;
        ListFiles::ls("/tmp", result);
        cout << result << endl;
        result.clear();
        ListFiles::ls("./main.cpp", result);
        cout << result << endl;
        result.clear();
        ListFiles::ls("/home/hzx/MyStudy/OS/FTP/Server/src/test/tools/OtherToolsTest.hpp", result);
        cout << result << endl;
    }

    static void testFS() {
        assert(FileSystem::mkDir("/tmp/1ttt"));
        assert(FileSystem::delFileAndDir("/tmp/1ttt"));
        assert(FileSystem::mkDir("/tmp/nDir"));
        assert(FileSystem::delFileAndDir("/tmp/nDir"));
        assert(FileSystem::isExistsAndReadable("/tmp/2ttt"));
        assert(!FileSystem::isPathAbsolute("."));
        PBB t = FileSystem::isDir("/tmp");
        assert(t.first && t.second);
    }
};

#endif //FTP_SERVER_OTHER_TOOLS_TEST_HPP
