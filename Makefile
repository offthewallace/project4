g++ TCPServerUtility.c DieWithMessage.c Server.cpp -o server -lcrypto -lpthread
 g++ TCPClientUtility.c DieWithMessage.c Client.cpp -o client -lcrypto