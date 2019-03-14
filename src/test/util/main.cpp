//
// Created by hzx on 19-3-8.
//

#include "UtilityTest.hpp"

int main() {
    // UtilityTest::testBug();
    // UtilityTest::testBugWithErrno();
    // UtilityTest::testMereString();
    // UtilityTest::testNumLen();
    // UtilityTest::testIO();
    // UtilityTest::testCondition(10);
    // UtilityTest::testConditionSignal();
    // UtilityTest::testConditionBroadcast();
    // UtilityTest::testMutex(10);
    // UtilityTest::testTimedMutex();
    // UtilityTest::testThread();
    // UtilityTest::testSignal();
    UtilityTest::testNetworkAndReadLine("8083", 1024);
    UtilityTest::testOpenListenFd();
    // UtilityTest::testAccept();
    // UtilityTest::testConsumeUntilEndOfLine();
    // UtilityTest::testGetUidGidHomeDir();
    // UtilityTest::testOpenDirAndCloseDir();
    // UtilityTest::testCloseDir();
}