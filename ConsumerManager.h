#pragma once
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <winsock2.h>
#include "json.hpp"
#include "Queue.h"
#include "ThreadPool.h"
#include "net/TcpServer.h"

class ConsumerManager {
    using json = nlohmann::json;
    fd_set masterSet;
    fd_set readSet;
    SOCKET maxSocket;

    // 存储消费者ID到订阅话题列表的映射
    std::unordered_map<SOCKET, std::unordered_set<std::string>> consumerTopics;

    // 存储话题到订阅消费者列表的映射（反向索引，便于查找）
    std::unordered_map<std::string, std::unordered_set<SOCKET>> topicConsumers;
    std::string ip;
    std::uint16_t port;
    std::shared_ptr<ThreadPool> threadPool;
    std::shared_ptr<Queue<json>> taskQueue;
    struct ClientInfo {
        std::string buffer; // 接收缓冲区
        std::chrono::steady_clock::time_point lastActivity;
    };
    std::unordered_map<SOCKET, ClientInfo> clients;
    std::atomic<bool> isRunning{false};
    TcpServer server;
    void handleSelect();
    void processClientData(SOCKET clientSocket);
    void removeClient(SOCKET clientSocket);
public:
    ConsumerManager(std::string ip,const std::uint16_t &port, const std::shared_ptr<ThreadPool> &threadPool,const std::shared_ptr<Queue<json>> &taskQueue):
    ip(std::move(ip)),port(port),threadPool(threadPool),taskQueue(taskQueue){};
    // 添加订阅关系
    void subscribe(const SOCKET &consumer, const std::string& topic);

    // 取消订阅
    void unsubscribe(const SOCKET &consumer, const std::string& topic);

    // 获取消费者订阅的所有话题
    std::unordered_set<std::string> getTopicsByConsumer(const SOCKET &consumer) const;

    // 获取订阅某个话题的所有消费者
    std::unordered_set<SOCKET> getConsumersByTopic(const std::string& topic) const;

    // 检查消费者是否订阅了特定话题
    bool isSubscribed(const SOCKET &consumer, const std::string& topic) const;
    void start();
    void handleClient(SOCKET clientSocket);
    void stop();
};
