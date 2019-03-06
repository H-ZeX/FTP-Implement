cmake_minimum_required(VERSION 3.13)
project(FTPServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_COMPILER /usr/bin/clang++-7)
add_compile_options(-W -Wall -g)

include_directories(.)

set(SRC
        src/util/def.h
        src/tools/fileSystem.cpp
        src/tools/fileSystem.h
        src/ftp/ftp.cpp
        src/ftp/ftp.h
        src/tools/ls.cpp
        src/tools/ls.h
        src/main.cpp
        src/util/netUtility.cpp
        src/util/netUtility.h
        src/ftp/networkSession.cpp
        src/ftp/networkSession.h
        src/ftp/prelogin.cpp
        src/ftp/prelogin.h
        src/ftp/session.cpp
        src/ftp/session.h
        src/tools/threadPool.cpp
        src/tools/threadPool.h
        src/util/threadUtility.cpp
        src/util/threadUtility.h
        src/util/utility.cpp
        src/util/utility.h)

add_executable(${PROJECT_NAME} ${SRC})
target_link_libraries(${PROJECT_NAME} -lpthread -rdynamic -lcrypt)
