cmake_minimum_required(VERSION 3.13)
project(FTPServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER /usr/bin/clang++-7)
#set(CMAKE_CXX_COMPILER /usr/bin/g++)
add_compile_options(-W -Wall -g)

include_directories(.)

set(SRC
        src/main/util/Def.hpp
        src/main/tools/FileSystem.hpp
        src/main/core/FTP.cpp
        src/main/core/FTP.hpp
        src/main/tools/ListFiles.hpp
        src/main/main.cpp
        src/main/util/NetUtility.hpp
        src/main/core/NetworkSession.hpp
        src/main/core/PreLogin.cpp
        src/main/core/Login.hpp
        src/main/core/Session.hpp
        src/main/tools/ThreadPool.hpp
        src/main/util/ThreadUtility.hpp
        src/main/util/Utility.hpp)

set(UtilTestSrc
        src/test/util/main.cpp
        src/test/util/UtilityTest.hpp
        )
set(ToolsTestSrc
        src/test/tools/main.cpp
        src/test/tools/ThreadPoolTest.hpp
        src/test/tools/OtherToolsTest.hpp)

add_executable(${PROJECT_NAME} ${SRC})
add_executable(UtilTest ${UtilTestSrc})
add_executable(ToolsTest ${ToolsTestSrc})

enable_testing()
add_test(UTest UtilTest)
add_test(TTest ToolsTest)

#target_compile_options(UtilTest PUBLIC -fprofile-arcs -ftest-coverage)
#target_compile_options(ToolsTest PUBLIC -fprofile-arcs -ftest-coverage)

target_link_libraries(${PROJECT_NAME} -lpthread -rdynamic -lcrypt)

#target_link_libraries(UtilTest -lpthread -rdynamic -lgcov --coverage)
#target_link_libraries(ToolsTest -lpthread -rdynamic -lgcov --coverage)
target_link_libraries(UtilTest -lpthread -rdynamic)
target_link_libraries(ToolsTest -lpthread -rdynamic)
