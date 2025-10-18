//
// Created by 13717 on 2025/10/1.
//

#include <stdexcept>
#include <iostream>
#include <utility>
#include "TcpSocket.h"
TcpSocket::TcpSocket(std::string  ip, int port)
    : ip(std::move(ip)), port(port)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock初始化失败，错误代码: " << WSAGetLastError() << std::endl;
    }
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Socket创建失败，错误代码: " << WSAGetLastError() << std::endl;
        WSACleanup();
    }

}
bool TcpSocket::connect() const
{
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddr.sin_port = htons(port);
    struct timeval timeout{};
    timeout.tv_sec = 5;  // 秒
    timeout.tv_usec = 0; // 微秒
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    if (::connect(sockfd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "连接失败，错误代码: " << WSAGetLastError() << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return false;
    }
    return true;
}
int TcpSocket::send(const std::string& data) const {
    int bytesSent = ::send(sockfd, data.c_str(), data.size(), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "发送数据失败，错误代码: " << WSAGetLastError() << std::endl;
    }
    return bytesSent;
}
//std::string TcpSocket::recv(int bufferSize) const {
//    char buffer[bufferSize];
//    int bytesReceived = ::recv(sockfd, buffer, bufferSize, 0);
//    if (bytesReceived == SOCKET_ERROR) {
//        std::cerr << "接收数据失败，错误代码: " << WSAGetLastError() << std::endl;
//        return "";
//    }
//    buffer[bytesReceived] = '\0';
//    if (bytesReceived == 0) {
//        std::cout << "对方已关闭连接" << std::endl;
//        return "";
//    }
//    return std::string(buffer, bytesReceived);
//}
TcpSocket::~TcpSocket() {
    closesocket(sockfd);
    WSACleanup();
}