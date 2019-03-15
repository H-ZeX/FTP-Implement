//
// Created by hzx on 19-3-15.
//

#ifndef FTP_SERVER_CORE_TEST_HPP
#define FTP_SERVER_CORE_TEST_HPP

#include <src/main/core/Login.hpp>

class CoreTest {
public:
    static void testLogin() {
        UserInfo me = login("hzx", "nhzsmjrsgdl", "", "");
        assert(me.isValid);
        cout << me.uid << " " << me.gid << " " << me.homeDir << endl;
        me = login("hzx", "ss", "", "");
        assert(!me.isValid);
        me = login("hz", "ed", "", "");
        assert(!me.isValid);
    }
};

#endif //FTP_SERVER_CORE_TEST_HPP
