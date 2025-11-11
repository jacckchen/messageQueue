#pragma once
#include <string>

#include "ClusterDispatcher.h"
#include "Receiver.h"
#include "common/config.h"
#include "ConsumerManager.h"
class Server {
    using json = nlohmann::json;
    std::string mode;
    std::string ip;
    uint16_t port;
    std::uint16_t receivePort;
    int threadPoolSize;
    std::shared_ptr<ThreadPool> threadPool;
    std::shared_ptr<ConsumerManager> consumerManager;
    std::shared_ptr<Queue<json>> messageQueue;
    std::shared_ptr<Queue<json>> taskQueue;
    std::unique_ptr<Receiver> receiver;
    std::unique_ptr<ClusterDispatcher> dispatcher;
    bool isRunning{true};
public:
    explicit Server(const Config &config);
    explicit Server(const std::string &configFile);
    void init();
    void start() const;
    void stop() const;
    ~Server();
};