from socket import *
import sys
import time

if len(sys.argv) < 4:
    print("usage: python3 violence_test.py portThatTheFTPServerListen yourSystemUserName yourSystemUserPassword")
    exit(0)

serverName = "127.0.0.1"
serverPort = int(sys.argv[1])
print(serverPort)
for i in range(0, 50000):
    clientSocket = socket(AF_INET, SOCK_STREAM)
    clientSocket.connect((serverName, serverPort))
    print(clientSocket)#, clientSocket.recv(1024));
    clientSocket.send(("user %s\r\n  pass %s\r\n stat .\r\n pwd\r\nport 127,0,0,1,111,111\r\nretr /home/hzx/tilix.dconf\r\n"%(sys.argv[2], sys.argv[3])).encode())
    clientSocket.close()
