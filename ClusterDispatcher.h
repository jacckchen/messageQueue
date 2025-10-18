//
// Created by 13717 on 2025/10/9.
//

#ifndef LIST_CLUSTERDISPATCHER_H
#define LIST_CLUSTERDISPATCHER_H
#include <atomic>

#include "ConsumerManager.h"
#include "MessageDispatcher.h"
#include "ThreadPool.h"


class ClusterDispatcher
{
    std::shared_ptr<Queue<json>> msgQueue_;                  // 待分发消息队列
    std::shared_ptr<ConsumerManager> consumerManager_; // 消费者管理器
    std::shared_ptr<ThreadPool> threadPool_;          // 线程池（用于异步分发）
    std::atomic<bool> isRunning_{false};              // 运行状态标志
    mutable std::unordered_map<std::string, size_t> topicRoundRobinIndex;
    void dispatchLoop();
    void dispatchMessage(const json& msg) const;
public:
    ClusterDispatcher(const std::shared_ptr<ConsumerManager> &consumerManager,
                     const std::shared_ptr<ThreadPool> &threadPool,
                     const std::shared_ptr<Queue<json>> &msgQueue)
        : msgQueue_(msgQueue),
          consumerManager_(consumerManager),
            threadPool_(threadPool) {}
    void start();

    void stop();
};


#endif //LIST_CLUSTERDISPATCHER_H