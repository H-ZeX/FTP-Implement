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
        src/main/core/FTP.cpp
        src/main/core/FTP.h
        src/main/tools/ListFiles.cpp
        src/main/tools/ListFiles.h
        src/main/main.cpp
        src/main/util/NetUtility.hpp
        src/main/core/NetworkSession.cpp
        src/main/core/NetworkSession.h
        src/main/core/PreLogin.cpp
        src/main/core/PreLogin.h
        src/main/core/Session.cpp
        src/main/core/Session.h
        src/main/tools/ThreadPool.hpp
        src/main/util/ThreadUtility.hpp
        src/main/util/Utility.hpp)

set(UtilTestSrc
        src/test/util/main.cpp
        src/test/util/UtilityTest.hpp
        src/main/util/Utility.hpp
        )

add_executable(${PROJECT_NAME} ${SRC})
add_executable(UtilTest ${UtilTestSrc})

enable_testing()
add_test(UTest UtilTest)

target_link_libraries(${PROJECT_NAME} -lpthread -rdynamic -lcrypt)

target_link_libraries(UtilTest -lpthread -rdynamic)
