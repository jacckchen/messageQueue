#pragma once
#include "ConsumerManager.h"
#include "Queue.h"
#include "common/config.h"

class BroadcastDispatcher {
private:
    std::shared_ptr<ConsumerManager> consumerManager;
    std::shared_ptr<ThreadPool> threadPool;
    std::shared_ptr<Queue<json>> messageQueue;
public:
    explicit BroadcastDispatcher(const std::shared_ptr<ConsumerManager> &consumerManager,
        const std::shared_ptr<ThreadPool> &threadPool,
        const std::shared_ptr<Queue<json>> &messageQueue):
    consumerManager(consumerManager),threadPool(threadPool),messageQueue(messageQueue){}
    void dispatch()
    {
        for ()
        std::unordered_set<SOCKET> consumers = consumerManager->getConsumersByTopic("");
    };
};
