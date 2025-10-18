// include/net/TcpServer.h
#pragma once
#include "TcpBase.h"
#include "../common/Logger.h"
#include <cstdint> // 用于uint16_t端口号

class TcpServer : public TcpBase {
private:
    sockaddr_in server_addr_{}; // 服务端地址信息

public:
    TcpServer() : TcpBase() {}

    // 绑定IP和端口（服务端特有）
    bool bind(const std::string& ip, uint16_t port) {
        if (!isValid()) return false;

        // 初始化地址结构
        server_addr_.sin_family = AF_INET;
        server_addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // 转换IP为网络字节序
        server_addr_.sin_port = htons(port); // 转换端口为网络字节序

        // 绑定
        if (::bind(sock_fd_, reinterpret_cast<SOCKADDR*>(&server_addr_), sizeof(server_addr_)) == SOCKET_ERROR) {
            LOG_ERROR("绑定失败（IP: " + ip + ", 端口: " + std::to_string(port) + "）");
            return false;
        }
        LOG_INFO("绑定成功（IP: " + ip + ", 端口: " + std::to_string(port) + "）");
        return true;
    }

    // 开始监听（服务端特有）
    bool listen(int backlog = 5) const
    {
        if (!isValid()) return false;

        if (::listen(sock_fd_, backlog) == SOCKET_ERROR) {
//            Logger::log(LogLevel::ERROR, "监听失败");
            return false;
        }
//        Logger::log(LogLevel::INFO, "开始监听...");
        return true;
    }

    // 接受客户端连接（返回一个新的socket用于与该客户端通信）
    SOCKET accept() const
    {
        if (!isValid()) return INVALID_SOCKET;

        sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_sock = ::accept(sock_fd_, reinterpret_cast<SOCKADDR*>(&client_addr), &client_addr_len);
        if (client_sock == INVALID_SOCKET) {
//            Logger::log(LogLevel::ERROR, "接受连接失败");
            return INVALID_SOCKET;
        }

        // 打印客户端信息（可选）
        std::string client_ip = inet_ntoa(client_addr.sin_addr);
        uint16_t client_port = ntohs(client_addr.sin_port);
//        Logger::log(LogLevel::INFO, "新客户端连接（IP: " + client_ip + ", 端口: " + std::to_string(client_port) + "）");
        return client_sock;
    }
    bool close()
    {
        this->is_valid_ = false;
        closesocket(sock_fd_);
        return true;
    }
};
