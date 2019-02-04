cmake_minimum_required(VERSION 3.13)
project(Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Server
        def.h
        fileSystem.cpp
        fileSystem.h
        ftp.cpp
        ftp.h
        ls.cpp
        ls.h
        main.cpp
        netUtility.cpp
        netUtility.h
        networkSession.cpp
        networkSession.h
        prelogin.cpp
        prelogin.h
        session.cpp
        session.h
        threadPool.cpp
        threadPool.h
        threadUtility.cpp
        threadUtility.h
        utility.cpp
        utility.h)
