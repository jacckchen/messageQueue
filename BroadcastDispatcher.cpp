//
// Created by 13717 on 2025/10/3.
//

#include "BroadcastDispatcher.h"
BroadcastDispatcher::BroadcastDispatcher(const std::shared_ptr<ConsumerManager> &consumerManager,
        const std::shared_ptr<ThreadPool> &threadPool,
        const std::shared_ptr<Queue<json>> &messageQueue,
        const std::shared_ptr<Queue<json>> &taskQueue):
consumerManager(consumerManager),threadPool(threadPool),messageQueue(messageQueue),taskQueue(taskQueue){}
void BroadcastDispatcher::start() {
    isRunning=true;
    threadPool->enqueue([this](){dispatchLoop();});
}
void BroadcastDispatcher::dispatchLoop() {
    while (isRunning && !taskQueue->Empty()) {
        if (messageQueue->Empty())
        {

            continue;
        }
        json msg;
        if ((msg = messageQueue->deQueue_front())!=nullptr) {
            dispatch();
        }
    }
}

void BroadcastDispatcher::dispatch(SOCKET socket, const std::string &topic,int startId,int endId) {
    std::unordered_set<SOCKET> consumers = consumerManager->getConsumersByTopic("");
    for (auto consumer : consumers) {
        const std::string &&r=msg.dump()+"\n";
        send(consumer, r.c_str(), static_cast<int>(r.size()), 0);
    }
}

void BroadcastDispatcher::stop() {
    isRunning=false;
}