#include "InitClientSocket.h"
#include <iostream>

int InitClientSocket(SOCKET &serverSocket, const char *IP,const int port){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock³õÊ¼»¯Ê§°Ü£¬´íÎó´úÂë: " << WSAGetLastError() << std::endl;
        return 1;
    }
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket´´½¨Ê§°Ü£¬´íÎó´úÂë: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(IP);
    serverAddr.sin_port = htons(port);
    struct timeval timeout{};
    timeout.tv_sec = 5;  // Ãë
    timeout.tv_usec = 0; // Î¢Ãë
    setsockopt(serverSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
    if (connect(serverSocket, (sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Á¬½ÓÊ§°Ü£¬´íÎó´úÂë: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    return 0;
}