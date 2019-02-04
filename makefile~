CC=clang++
CFLAGS=-I. -std=c++11 -W -Wall -g
DEPS = utility.h netUtility.h session.h networkSession.h ls.h fileSystem.h prelogin.h ftp.h threadUtility.h threadPool.h

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

hzxftp: main.o ftp.o threadUtility.o threadPool.o utility.o netUtility.o session.o networkSession.o ls.o fileSystem.o prelogin.o 
	$(CC) -lpthread -lcrypt  -rdynamic -o  hzxftp main.o threadUtility.o threadPool.o ftp.o utility.o netUtility.o session.o networkSession.o ls.o fileSystem.o prelogin.o

clean: 
	rm *.o
