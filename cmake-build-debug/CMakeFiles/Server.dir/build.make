# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /media/hzx/infrastructure/BIN/clion/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /media/hzx/infrastructure/BIN/clion/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/hzx/MyStudy/OS/FTP/Server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/Server.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Server.dir/flags.make

CMakeFiles/Server.dir/fileSystem.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/fileSystem.cpp.o: ../fileSystem.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/Server.dir/fileSystem.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/fileSystem.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/fileSystem.cpp

CMakeFiles/Server.dir/fileSystem.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/fileSystem.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/fileSystem.cpp > CMakeFiles/Server.dir/fileSystem.cpp.i

CMakeFiles/Server.dir/fileSystem.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/fileSystem.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/fileSystem.cpp -o CMakeFiles/Server.dir/fileSystem.cpp.s

CMakeFiles/Server.dir/ftp.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/ftp.cpp.o: ../ftp.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/Server.dir/ftp.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/ftp.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/ftp.cpp

CMakeFiles/Server.dir/ftp.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/ftp.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/ftp.cpp > CMakeFiles/Server.dir/ftp.cpp.i

CMakeFiles/Server.dir/ftp.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/ftp.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/ftp.cpp -o CMakeFiles/Server.dir/ftp.cpp.s

CMakeFiles/Server.dir/ls.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/ls.cpp.o: ../ls.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object CMakeFiles/Server.dir/ls.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/ls.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/ls.cpp

CMakeFiles/Server.dir/ls.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/ls.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/ls.cpp > CMakeFiles/Server.dir/ls.cpp.i

CMakeFiles/Server.dir/ls.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/ls.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/ls.cpp -o CMakeFiles/Server.dir/ls.cpp.s

CMakeFiles/Server.dir/main.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/main.cpp.o: ../main.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object CMakeFiles/Server.dir/main.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/main.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/main.cpp

CMakeFiles/Server.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/main.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/main.cpp > CMakeFiles/Server.dir/main.cpp.i

CMakeFiles/Server.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/main.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/main.cpp -o CMakeFiles/Server.dir/main.cpp.s

CMakeFiles/Server.dir/netUtility.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/netUtility.cpp.o: ../netUtility.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object CMakeFiles/Server.dir/netUtility.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/netUtility.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/netUtility.cpp

CMakeFiles/Server.dir/netUtility.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/netUtility.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/netUtility.cpp > CMakeFiles/Server.dir/netUtility.cpp.i

CMakeFiles/Server.dir/netUtility.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/netUtility.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/netUtility.cpp -o CMakeFiles/Server.dir/netUtility.cpp.s

CMakeFiles/Server.dir/networkSession.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/networkSession.cpp.o: ../networkSession.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object CMakeFiles/Server.dir/networkSession.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/networkSession.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/networkSession.cpp

CMakeFiles/Server.dir/networkSession.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/networkSession.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/networkSession.cpp > CMakeFiles/Server.dir/networkSession.cpp.i

CMakeFiles/Server.dir/networkSession.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/networkSession.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/networkSession.cpp -o CMakeFiles/Server.dir/networkSession.cpp.s

CMakeFiles/Server.dir/prelogin.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/prelogin.cpp.o: ../prelogin.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object CMakeFiles/Server.dir/prelogin.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/prelogin.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/prelogin.cpp

CMakeFiles/Server.dir/prelogin.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/prelogin.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/prelogin.cpp > CMakeFiles/Server.dir/prelogin.cpp.i

CMakeFiles/Server.dir/prelogin.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/prelogin.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/prelogin.cpp -o CMakeFiles/Server.dir/prelogin.cpp.s

CMakeFiles/Server.dir/session.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/session.cpp.o: ../session.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object CMakeFiles/Server.dir/session.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/session.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/session.cpp

CMakeFiles/Server.dir/session.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/session.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/session.cpp > CMakeFiles/Server.dir/session.cpp.i

CMakeFiles/Server.dir/session.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/session.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/session.cpp -o CMakeFiles/Server.dir/session.cpp.s

CMakeFiles/Server.dir/threadPool.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/threadPool.cpp.o: ../threadPool.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Building CXX object CMakeFiles/Server.dir/threadPool.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/threadPool.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/threadPool.cpp

CMakeFiles/Server.dir/threadPool.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/threadPool.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/threadPool.cpp > CMakeFiles/Server.dir/threadPool.cpp.i

CMakeFiles/Server.dir/threadPool.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/threadPool.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/threadPool.cpp -o CMakeFiles/Server.dir/threadPool.cpp.s

CMakeFiles/Server.dir/threadUtility.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/threadUtility.cpp.o: ../threadUtility.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_10) "Building CXX object CMakeFiles/Server.dir/threadUtility.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/threadUtility.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/threadUtility.cpp

CMakeFiles/Server.dir/threadUtility.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/threadUtility.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/threadUtility.cpp > CMakeFiles/Server.dir/threadUtility.cpp.i

CMakeFiles/Server.dir/threadUtility.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/threadUtility.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/threadUtility.cpp -o CMakeFiles/Server.dir/threadUtility.cpp.s

CMakeFiles/Server.dir/utility.cpp.o: CMakeFiles/Server.dir/flags.make
CMakeFiles/Server.dir/utility.cpp.o: ../utility.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_11) "Building CXX object CMakeFiles/Server.dir/utility.cpp.o"
	/usr/bin/clang++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/Server.dir/utility.cpp.o -c /home/hzx/MyStudy/OS/FTP/Server/utility.cpp

CMakeFiles/Server.dir/utility.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Server.dir/utility.cpp.i"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/hzx/MyStudy/OS/FTP/Server/utility.cpp > CMakeFiles/Server.dir/utility.cpp.i

CMakeFiles/Server.dir/utility.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Server.dir/utility.cpp.s"
	/usr/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/hzx/MyStudy/OS/FTP/Server/utility.cpp -o CMakeFiles/Server.dir/utility.cpp.s

# Object files for target Server
Server_OBJECTS = \
"CMakeFiles/Server.dir/fileSystem.cpp.o" \
"CMakeFiles/Server.dir/ftp.cpp.o" \
"CMakeFiles/Server.dir/ls.cpp.o" \
"CMakeFiles/Server.dir/main.cpp.o" \
"CMakeFiles/Server.dir/netUtility.cpp.o" \
"CMakeFiles/Server.dir/networkSession.cpp.o" \
"CMakeFiles/Server.dir/prelogin.cpp.o" \
"CMakeFiles/Server.dir/session.cpp.o" \
"CMakeFiles/Server.dir/threadPool.cpp.o" \
"CMakeFiles/Server.dir/threadUtility.cpp.o" \
"CMakeFiles/Server.dir/utility.cpp.o"

# External object files for target Server
Server_EXTERNAL_OBJECTS =

Server: CMakeFiles/Server.dir/fileSystem.cpp.o
Server: CMakeFiles/Server.dir/ftp.cpp.o
Server: CMakeFiles/Server.dir/ls.cpp.o
Server: CMakeFiles/Server.dir/main.cpp.o
Server: CMakeFiles/Server.dir/netUtility.cpp.o
Server: CMakeFiles/Server.dir/networkSession.cpp.o
Server: CMakeFiles/Server.dir/prelogin.cpp.o
Server: CMakeFiles/Server.dir/session.cpp.o
Server: CMakeFiles/Server.dir/threadPool.cpp.o
Server: CMakeFiles/Server.dir/threadUtility.cpp.o
Server: CMakeFiles/Server.dir/utility.cpp.o
Server: CMakeFiles/Server.dir/build.make
Server: CMakeFiles/Server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_12) "Linking CXX executable Server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Server.dir/build: Server

.PHONY : CMakeFiles/Server.dir/build

CMakeFiles/Server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Server.dir/clean

CMakeFiles/Server.dir/depend:
	cd /home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/hzx/MyStudy/OS/FTP/Server /home/hzx/MyStudy/OS/FTP/Server /home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug /home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug /home/hzx/MyStudy/OS/FTP/Server/cmake-build-debug/CMakeFiles/Server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Server.dir/depend

