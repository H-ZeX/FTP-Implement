//
// Created by root on 19-3-17.
//

#ifndef CORE_TEST_CONFIG_HPP
#define CORE_TEST_CONFIG_HPP

#define CORE_TEST_USERNAME "hzx"
#define CORE_TEST_PASSWORD "nhzsmjrsgdl"
#define CORE_TEST_LISTEN_PORT_1 "8080"

/*
 * The test file should NOT be too large(about 1k),
 * because the test program is single thread,
 * so it should send all file before read.
 * So if the TCP buf can not contain all the data,
 * the send will block, so the program will block
 *
 * These FILE and DIR macro should be very careful,
 * the program may write to them
 *
 */
/*
 * before test, this file should exist
 */
#define CORE_TEST_LOCAL_FILE_1 "/tmp/nesdasflwejdlejkf1"
#define CORE_TEST_LOCAL_FILE_2 "/tmp/nesdasflwejdlejknewF1"

/*
 * before test, this dir should exist
 */
#define CORE_TEST_LOCAL_DIR_1 "/tmp/nesdasflwejdlejktdir"
#define CORE_TEST_LOCAL_DIR_2 "/tmp/nesdasflwejdlejktdir_2/"

#define CORE_TEST_FTP_LISTEN_PORT 8001

#endif //CORE_TEST_CONFIG_HPP
