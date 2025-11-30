#pragma once
#include <memory>

#include "json.hpp"
#include "Queue.h"
#include "ThreadPool.h"
#include "net/TcpServer.h"

class Receiver {
    using json = nlohmann::json;
    std::string ip;
    std::uint16_t port;
    std::shared_ptr<Queue<json>> messageQueue;
    std::shared_ptr<ThreadPool> threadPool;
    TcpServer server;
    fd_set masterSet{};
    fd_set readSet{};
    SOCKET maxSocket;
    std::atomic<bool> isRunning{true};
    struct ClientInfo {
        std::string buffer; // 接收缓冲区
        std::chrono::steady_clock::time_point lastActivity;
    };
    std::unordered_map<SOCKET, ClientInfo> clients;
    void handleSelect();
public:
    Receiver(std::string ip, const std::uint16_t port,
        const std::shared_ptr<Queue<json>> &messageQueue,
        const std::shared_ptr<ThreadPool> &threadPool) :
        ip(std::move(ip)), port(port), messageQueue(messageQueue), threadPool(threadPool),
        maxSocket(0)
    {
    };
    void receive();
    void stop();
private:
    void handleClient(SOCKET clientSocket);

};
