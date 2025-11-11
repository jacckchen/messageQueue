#pragma once
#include "ConsumerManager.h"
#include "Queue.h"
#include "common/config.h"

class BroadcastDispatcher {
private:
    std::shared_ptr<ConsumerManager> consumerManager;
    std::shared_ptr<ThreadPool> threadPool;
    std::shared_ptr<Queue<json>> messageQueue;
    std::shared_ptr<Queue<json>> taskQueue;
    bool isRunning{false};
    void dispatchLoop();
    void dispatch(SOCKET clientSocket, const std::string &topic,int startId,int endId);
public:
    explicit BroadcastDispatcher(const std::shared_ptr<ConsumerManager> &consumerManager,
        const std::shared_ptr<ThreadPool> &threadPool,
        const std::shared_ptr<Queue<json>> &messageQueue,
        const std::shared_ptr<Queue<json>> &taskQueue);
    void start();
    void stop();
};
