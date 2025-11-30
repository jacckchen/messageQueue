#include "Receiver.h"

#include <iostream>

std::atomic<int> co = 0;
std::mutex mtx;


void Receiver::receive()
{
    // 绑定和监听已在构造函数中完成，此处直接开始接收连接
    if (!server.bind(this->ip, this->port))
    {
        std::cerr << "Failed to bind to " << ip << ":" << port << std::endl;
        return;
    }

    if (!server.listen())
    {
        std::cerr << "Failed to listen on " << ip << ":" << port << std::endl;
        return;
    }

    FD_ZERO(&masterSet);
    FD_SET(server.getSocket(), &masterSet);
    while (isRunning)
    {
        handleSelect();
    }
}

void Receiver::handleSelect()
{
    timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    readSet = masterSet;
    int activity = select(maxSocket + 1, &readSet, nullptr, nullptr, &timeout);
    if (activity < 0)
    {
        LOG_ERROR("select error");
        perror("select error");
        return;
    }
    if (activity == 0)
    {
        for (auto it = clients.begin(); it != clients.end();)
        {
            // 超时检查，清理不活跃的客户端
            const auto now = std::chrono::steady_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.lastActivity).count();

            if (duration > 300) // 5分钟无活动则断开连接
            {
                LOG_INFO("Client timeout, closing connection");
                closesocket(it->first);
                FD_CLR(it->first, &masterSet);

                it = clients.erase(it);
            }
            else
            {
                ++it;
            }
        }
        return;
    }
    if (const SOCKET listenSocket = server.getSocket(); FD_ISSET(listenSocket, &readSet))
    {
        if (const SOCKET clientSocket = server.accept(); clientSocket != INVALID_SOCKET)
        {
            FD_SET(clientSocket, &masterSet);
            if (clientSocket > maxSocket)
                maxSocket = clientSocket;

            // 初始化客户端信息
            clients[clientSocket] = ClientInfo{"", std::chrono::steady_clock::now()};

            LOG_INFO("New client connected: " + std::to_string(clientSocket));
        }

        activity--;
    }

    // 处理已连接的客户端
    for (auto it = clients.begin(); it != clients.end() && activity > 0;)
    {
        if (const SOCKET clientSocket = it->first; FD_ISSET(clientSocket, &readSet))
        {
            activity--;
            handleClient(clientSocket);
        }

        ++it;
    }
}

void Receiver::handleClient(const SOCKET clientSocket)
{
    char buffer[1024];
    const int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        clients[clientSocket].buffer += buffer; // 将新数据追加到缓冲区
        // LOG_DEBUG("收到原始数据（长度：" + std::to_string(bytesReceived) + "）：" + std::string(buffer));

        // 按回车拆分缓冲区中的完整JSON
        size_t pos;
        while ((pos = clients[clientSocket].buffer.find('\n')) != std::string::npos)
        {
            // 提取一条完整的JSON（从开头到换行符）
            std::string jsonStr = clients[clientSocket].buffer.substr(0, pos);
            // 移除已处理的部分（包括换行符）
            clients[clientSocket].buffer.erase(0, pos + 1);

            // 解析并处理JSON（忽略空字符串，避免空行干扰）
            if (!jsonStr.empty())
            {
                try
                {
                    json message = json::parse(jsonStr);
                    messageQueue->enQueue_back(message);
                    ++co;
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        if (co == 10000)
                        {
                            co = 0;
                            LOG_INFO("一万条");
                        }
                    }

                    // LOG_INFO("解析并添加消息：" + jsonStr);
                }
                catch (const json::parse_error& e)
                {
                    LOG_ERROR("JSON解析失败（内容：" + jsonStr + "），错误：" + std::string(e.what()));
                }
            }
        }
    }
    else if (bytesReceived == 0)
    {
        LOG_INFO("客户端断开连接，剩余未解析数据：" + clients[clientSocket].buffer);
    }
    else
    {
        if (const int err = WSAGetLastError(); err != WSAETIMEDOUT)
        {
            LOG_ERROR("接收数据错误（错误码：" + std::to_string(err) + "），剩余未解析数据：" + clients[clientSocket].buffer);
        }
    }
}

void Receiver::stop()
{
    isRunning = false;
    server.close();
}
