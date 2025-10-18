// include/net/TcpBase.h
#pragma once
#include <winsock2.h> // 若用Linux则替换为 <sys/socket.h>
#include <string>
//#include "../common/Logger.h" // 之前建议的日志类

// 注意：Windows需要链接ws2_32.lib，可在代码中添加
#pragma comment(lib, "ws2_32.lib")

class TcpBase {
protected:
    SOCKET sock_fd_; // 底层socket句柄
    bool is_valid_;  // 标记socket是否有效（避免重复关闭）

    // 初始化Socket环境（仅Windows需要，Linux可省略）
    void initWinsock() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//            Logger::log(LogLevel::ERROR, "Winsock初始化失败");
            is_valid_ = false;
        }
#endif
    }

    // 清理Socket环境（仅Windows需要）
    void cleanupWinsock() {
#ifdef _WIN32
        WSACleanup();
#endif
    }

public:
    // 构造函数：创建基础socket
    TcpBase() : sock_fd_(INVALID_SOCKET), is_valid_(true) {
        initWinsock();
        // 创建TCP socket（IPV4 + 字节流）
        sock_fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_fd_ == INVALID_SOCKET) {
//            Logger::log(LogLevel::ERROR, "创建socket失败");
            is_valid_ = false;
        }
    }

    // 析构函数：自动关闭socket（RAII原则）
    virtual ~TcpBase() {
        if (sock_fd_ != INVALID_SOCKET) {
            closesocket(sock_fd_); // Linux下为 close(sock_fd_)
//            Logger::log(LogLevel::INFO, "socket已关闭");
        }
        cleanupWinsock();
    }

    // 禁止拷贝（避免socket句柄重复关闭）
    TcpBase(const TcpBase&) = delete;
    TcpBase& operator=(const TcpBase&) = delete;

    // 判断socket是否有效
    [[nodiscard]] bool isValid() const { return is_valid_; }

    // 获取底层socket句柄（谨慎使用，避免绕过封装）
    [[nodiscard]] SOCKET getSocket() const { return sock_fd_; }
};
