cmake_minimum_required(VERSION 3.13)
project(FTPServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER /usr/bin/clang++-7)
add_compile_options(-W -Wall -g)

include_directories(.)

set(SRC
        src/main/util/Def.hpp
        src/main/tools/FileSystem.cpp
        src/main/tools/FileSystem.h
        src/main/ftp/FTP.cpp
        src/main/ftp/FTP.h
        src/main/tools/List.cpp
        src/main/tools/List.h
        src/main/main.cpp
        src/main/util/NetUtility.hpp
        src/main/ftp/NetworkSession.cpp
        src/main/ftp/NetworkSession.h
        src/main/ftp/PreLogin.cpp
        src/main/ftp/PreLogin.h
        src/main/ftp/Session.cpp
        src/main/ftp/Session.h
        src/main/tools/ThreadPool.cpp
        src/main/tools/ThreadPool.h
        src/main/util/ThreadUtility.hpp
        src/main/util/utility.hpp)

set(UtilTestSrc
        src/test/util/main.cpp
        src/test/util/UtilityTest.hpp
        src/main/util/utility.hpp
        )

add_executable(${PROJECT_NAME} ${SRC})
add_executable(UtilTest ${UtilTestSrc})

enable_testing()
add_test(UTest UtilTest)

target_link_libraries(${PROJECT_NAME} -lpthread -rdynamic -lcrypt)

target_link_libraries(UtilTest -lpthread -rdynamic)
