
#include "Receiver.h"

#include <iostream>

void Receiver::receive() {
    // 绑定和监听已在构造函数中完成，此处直接开始接收连接
    if (!server.bind(this->ip, this->port)) {
        std::cerr << "Failed to bind to " << ip << ":" << port << std::endl;
        return;
    }

    if (!server.listen()) {
        std::cerr << "Failed to listen on " << ip << ":" << port << std::endl;
        return;
    }

    while (isRunning) {
        SOCKET clientSocket = server.accept();
        if (clientSocket != INVALID_SOCKET) {
            // 为每个客户端创建新线程处理
            threadPool->enqueue([this, clientSocket]() {
                handleClient(clientSocket);
            });
        }
    }
}
void Receiver::handleClient(SOCKET clientSocket) const
{
    char buffer[1024];
    std::string recvBuffer;  // 累积接收数据的缓冲区
    recvBuffer.reserve(1024);
    while (isRunning) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            recvBuffer += buffer;  // 将新数据追加到缓冲区
            // LOG_DEBUG("收到原始数据（长度：" + std::to_string(bytesReceived) + "）：" + std::string(buffer));

            // 按回车拆分缓冲区中的完整JSON
            size_t pos;
            while ((pos = recvBuffer.find('\n')) != std::string::npos) {
                // 提取一条完整的JSON（从开头到换行符）
                std::string jsonStr = recvBuffer.substr(0, pos);
                // 移除已处理的部分（包括换行符）
                recvBuffer.erase(0, pos + 1);

                // 解析并处理JSON（忽略空字符串，避免空行干扰）
                if (!jsonStr.empty()) {
                    try {
                        json message = json::parse(jsonStr);
                        messageQueue->enQueue_back(message);
                        // LOG_INFO("解析并添加消息：" + jsonStr);
                    } catch (const json::parse_error& e) {
                        LOG_ERROR("JSON解析失败（内容：" + jsonStr + "），错误：" + std::string(e.what()));
                    }
                }
            }
        } else if (bytesReceived == 0) {
            LOG_INFO("客户端断开连接，剩余未解析数据：" + recvBuffer);
            break;
        } else {
            int err = WSAGetLastError();
            if (err != WSAETIMEDOUT) {
                LOG_ERROR("接收数据错误（错误码：" + std::to_string(err) + "），剩余未解析数据：" + recvBuffer);
                break;
            }
        }
    }
    // 关闭客户端连接
    closesocket(clientSocket);
}
void Receiver::stop()
{
    isRunning = false;
    server.close();
}
