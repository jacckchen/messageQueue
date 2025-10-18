// include/net/TcpClient.h
#pragma once
#include "TcpBase.h"
#include <cstdint>

class TcpClient : public TcpBase {
private:
    sockaddr_in server_addr_{}; // 目标服务端地址

public:
    TcpClient() : TcpBase() {

    }

    // 连接到服务端（客户端特有）
    bool connect(const std::string& server_ip, uint16_t server_port) {
        if (!isValid()) return false;

        // 初始化服务端地址
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = inet_addr(server_ip.c_str());
        server_addr_.sin_port = htons(server_port);

        struct timeval timeout{};
        timeout.tv_sec = 5;  // 秒
        timeout.tv_usec = 0; // 微秒
        setsockopt(sock_fd_, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
        // 发起连接
        if (::connect(sock_fd_, (SOCKADDR*)&server_addr_, sizeof(server_addr_)) == SOCKET_ERROR) {
//            Logger::log(LogLevel::ERROR, "连接服务端失败（IP: " + server_ip + ", 端口: " + std::to_string(server_port) + "）");
            return false;
        }
//        Logger::log(LogLevel::INFO, "成功连接到服务端");
        return true;
    }

    // 发送数据（客户端和服务端都可能用到，放在客户端类中，服务端处理客户端连接时也可调用）
    int send(const std::string& data) {
        if (!isValid()) return -1;

        int bytes_sent = ::send(sock_fd_, data.c_str(), data.size(), 0);
        if (bytes_sent == SOCKET_ERROR) {
//            Logger::log(LogLevel::ERROR, "发送数据失败");
            return -1;
        }
        return bytes_sent;
    }

    // 接收数据
    std::string recv(int buffer_size = 1024) {
        if (!isValid()) return "";

        char buffer[buffer_size];
        int bytes_recv = ::recv(sock_fd_, buffer, buffer_size - 1, 0); // 留1字节给'\0'
        if (bytes_recv <= 0) {
            if (bytes_recv == 0) {
//                Logger::log(LogLevel::INFO, "连接已关闭");
            } else {
//                Logger::log(LogLevel::ERROR, "接收数据失败");
            }
            return "";
        }

        buffer[bytes_recv] = '\0'; // 确保字符串结束
        return std::string(buffer);
    }
};
