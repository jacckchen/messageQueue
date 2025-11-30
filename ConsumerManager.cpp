#include "ConsumerManager.h"

#include "net/TcpServer.h"
#include <chrono>
#include <thread>
void ConsumerManager::start()
{
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
    isRunning = true;
    FD_ZERO(&masterSet);
    FD_SET(server.getSocket(), &masterSet);
    // while (isRunning)
    // {
    //     SOCKET clientSocket = server.accept();
    //
    //     if (clientSocket != INVALID_SOCKET)
    //     {
    //         threadPool->enqueue([this, clientSocket]()
    //         {
    //             handleClient(clientSocket);
    //         });
    //     }
    // }
    while (isRunning)
    {
        handleSelect();
    }
}

// void ConsumerManager::handleClient(const SOCKET clientSocket)
// {
//     char buffer[1024];
//     std::string recvBuffer;
//     recvBuffer.reserve(1024);
//     int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
//     if (bytesReceived > 0)
//     {
//         buffer[bytesReceived] = '\0';
//         recvBuffer += buffer; // 将新数据追加到缓冲区
//         LOG_DEBUG("收到原始数据（长度：" + std::to_string(bytesReceived) + "）：" + std::string(buffer));
//
//         // 按回车拆分缓冲区中的完整JSON
//         size_t pos;
//         while ((pos = recvBuffer.find('\n')) != std::string::npos)
//         {
//             // 提取一条完整的JSON（从开头到换行符）
//             std::string jsonStr = recvBuffer.substr(0, pos);
//             // 移除已处理的部分（包括换行符）
//             recvBuffer.erase(0, pos + 1);
//
//             // 解析并处理JSON（忽略空字符串，避免空行干扰）
//             if (!jsonStr.empty())
//             {
//                 try
//                 {
//                     json message = json::parse(jsonStr);
//                     if (message["type"] == "Register")
//                     {
//                         LOG_INFO("添加客户端：" + jsonStr);
//                         subscribe(clientSocket, message["body"]["RequireTag"]);
//                         continue;
//                     }
//                     if (message["type"] == "Unregister")
//                     {
//                         LOG_INFO("删除客户端：" + jsonStr);
//                         unsubscribe(clientSocket, message["body"]["RequireTag"]);
//                         continue;
//                     }
//                     if (message["type"] == "Pull")
//                     {
//                         taskQueue->enQueue_back(message);
//                     }
//                     LOG_INFO("从客户端接收消息：" + jsonStr);
//                 }
//                 catch (const json::parse_error& e)
//                 {
//                     LOG_ERROR("JSON解析失败（内容：" + jsonStr + "），错误：" + std::string(e.what()));
//                 }
//             }
//         }
//     }
//     else if (bytesReceived == 0)
//     {
//         LOG_INFO("客户端断开连接，剩余未解析数据：" + recvBuffer);
//     }
//     else
//     {
//         int err = WSAGetLastError();
//         if (err != WSAETIMEDOUT)
//         {
//             LOG_ERROR("接收数据错误（错误码：" + std::to_string(err) + "），剩余未解析数据：" + recvBuffer);
//         }
//     }
// }
void ConsumerManager::handleSelect()
{
    timeval timeout{};
    timeout.tv_sec = 1;  // 1秒超时
    timeout.tv_usec = 0;

    readSet = masterSet;

    int activity = select(static_cast<int>(maxSocket + 1), &readSet, nullptr, nullptr, &timeout);

    if (activity < 0)
    {
        if (isRunning)
        {
            LOG_ERROR("select error");
        }
        return;
    }

    if (activity == 0)
    {
        for (auto it = clients.begin(); it != clients.end(); )
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

                // 从订阅表中移除客户端
                for (auto topics = getTopicsByConsumer(it->first); const auto& topic : topics)
                {
                    unsubscribe(it->first, topic);
                }

                it = clients.erase(it);
            }
            else
            {
                ++it;
            }
        }
        return;
    }

    // 检查监听套接字是否有新连接
    if (const SOCKET listenSocket = server.getSocket(); FD_ISSET(listenSocket, &readSet))
    {
        if (const SOCKET clientSocket = server.accept(); clientSocket != INVALID_SOCKET)
        {
            FD_SET(clientSocket, &masterSet);
            if (clientSocket > maxSocket)
                maxSocket = clientSocket;

            // 初始化客户端信息
            clients[clientSocket] = ClientInfo{ "", std::chrono::steady_clock::now() };

            LOG_INFO("New client connected: " + std::to_string(clientSocket));
        }

        activity--;
    }

    // 处理已连接的客户端
    for (auto it = clients.begin(); it != clients.end() && activity > 0; )
    {
        if (const SOCKET clientSocket = it->first; FD_ISSET(clientSocket, &readSet))
        {
            activity--;
            processClientData(clientSocket);
        }

        ++it;
    }
}
void ConsumerManager::processClientData(const SOCKET clientSocket)
{
    char buffer[1024];

    if (const int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0); bytesReceived > 0)
    {
        // 更新最后活动时间
        clients[clientSocket].lastActivity = std::chrono::steady_clock::now();

        buffer[bytesReceived] = '\0';
        clients[clientSocket].buffer += buffer;

        LOG_DEBUG("Received raw data (length: " + std::to_string(bytesReceived) + "): " + std::string(buffer));

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
                    if (message["type"] == "Register")
                    {
                        LOG_INFO("Adding client: " + jsonStr);
                        subscribe(clientSocket, message["body"]["RequireTag"]);
                        continue;
                    }
                    if (message["type"] == "Unregister")
                    {
                        LOG_INFO("Removing client: " + jsonStr);
                        unsubscribe(clientSocket, message["body"]["RequireTag"]);
                        continue;
                    }
                    if (message["type"] == "Pull")
                    {
                        taskQueue->enQueue_back(message);
                    }
                    LOG_INFO("Message received from client: " + jsonStr);
                }
                catch (const json::parse_error& e)
                {
                    LOG_ERROR("JSON parsing failed (content: " + jsonStr + "), error: " + std::string(e.what()));
                }
            }
        }
    }

    else if (bytesReceived == 0)
    {
        // 客户端关闭连接
        LOG_INFO("Client disconnected, remaining unparsed data: " + clients[clientSocket].buffer);
        removeClient(clientSocket);
    }
    else
    {
        if (const int err = WSAGetLastError(); err != WSAETIMEDOUT)
        {
            LOG_ERROR("Receive data error (error code: " + std::to_string(err) + "), remaining unparsed data: " + clients[clientSocket].buffer);
            removeClient(clientSocket);
        }
    }
}
void ConsumerManager::removeClient(const SOCKET clientSocket)
{
    // 从订阅表中移除客户端
    for (const auto topics = getTopicsByConsumer(clientSocket); const auto& topic : topics)
    {
        unsubscribe(clientSocket, topic);
    }

    // 关闭套接字并从集合中移除
    closesocket(clientSocket);
    FD_CLR(clientSocket, &masterSet);
    clients.erase(clientSocket);

    // 更新最大套接字值
    if (clientSocket == maxSocket)
    {
        maxSocket = server.getSocket();
        for (const auto& key : clients|std::views::keys)
        {
            if (key > maxSocket)
                maxSocket = key;
        }
    }
}

void ConsumerManager::stop()
{
    isRunning = false;
    for (const auto& socket : consumerTopics | std::views::keys)
    {
        closesocket(socket);
    }
    server.close();
}

void ConsumerManager::subscribe(const SOCKET& consumer, const std::string& topic)
{
    consumerTopics[consumer].insert(topic);
    topicConsumers[topic].insert(consumer);
}

void ConsumerManager::unsubscribe(const SOCKET& consumer, const std::string& topic)
{
    if (const auto consumerIt = consumerTopics.find(consumer); consumerIt != consumerTopics.end())
    {
        consumerIt->second.erase(topic);
        if (consumerIt->second.empty())
        {
            consumerTopics.erase(consumerIt);
        }
    }

    if (const auto topicIt = topicConsumers.find(topic); topicIt != topicConsumers.end())
    {
        topicIt->second.erase(consumer);
        if (topicIt->second.empty())
        {
            topicConsumers.erase(topicIt);
        }
    }
}

std::unordered_set<std::string> ConsumerManager::getTopicsByConsumer(const SOCKET& consumer) const
{
    if (const auto it = consumerTopics.find(consumer); it != consumerTopics.end())
    {
        return it->second;
    }
    return {};
}

std::unordered_set<SOCKET> ConsumerManager::getConsumersByTopic(const std::string& topic) const
{
    if (const auto it = topicConsumers.find(topic); it != topicConsumers.end())
    {
        return it->second;
    }
    return {};
}

bool ConsumerManager::isSubscribed(const SOCKET& consumer, const std::string& topic) const
{
    if (const auto it = consumerTopics.find(consumer); it != consumerTopics.end())
    {
        return it->second.contains(topic);
    }
    return false;
}
