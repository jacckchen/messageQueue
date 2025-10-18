#pragma once

#include <winsock.h>
#pragma comment(lib, "ws2_32.lib")
#include <string>
class TcpSocket {
private:
    SOCKET sockfd;
    std::string ip;
    int port;
public:
    TcpSocket(std::string  ip, int port);
    ~TcpSocket(); // 自动关闭socket，避免资源泄漏
    bool connect() const; // 封装connect逻辑
    [[nodiscard]] int send(const std::string& data) const; // 封装send，处理错误
    [[nodiscard]] std::string recv(int bufferSize = 1024) const; // 封装recv，返回字符串
    // 禁止拷贝，避免socket重复关闭
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
};
