//
// Created by root on 19-3-17.
//

#include "src/main/core/FTP.hpp"

class FTPTest {
public:
    static void test1() {
        FTP *ftp = FTP::getInstance(8001, 4);
        ftp->startAndRun();
    }
};
