#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "ConsumerManager.h"
#include "json.hpp"
#include <winsock2.h>

#include "Queue.h"
#include "common/Logger.h"

using json = nlohmann::json;



class MessageDispatcher {
private:
    std::shared_ptr<Queue<json>> msgQueue_;                  // 待分发消息队列
    std::shared_ptr<ConsumerManager> consumerManager_; // 消费者管理器
    std::shared_ptr<ThreadPool> threadPool_;          // 线程池（用于异步分发）
    std::atomic<bool> isRunning_{false};              // 运行状态标志

    // 消息分发工作函数
    void dispatchLoop() const
    {
        while (isRunning_) {
            json msg;
            if ((msg = msgQueue_->deQueue_front())!=nullptr) {
                dispatchMessage(msg);
            }
        }
    }

    // 实际分发单条消息
    void dispatchMessage(const json& msg) const
    {
        try {
            if (!msg.contains("type") || !msg.contains("tag")) {
                // spdlog::warn("Invalid message format: missing topic or content");
                return;
            }

            const std::string topic = msg["tag"];
            // 获取订阅该话题的所有消费者
            auto consumers = consumerManager_->getConsumersByTopic(topic);

            if (consumers.empty()) {
                // LOG_WARN("No consumers for topic: {}");
                return;
            }

            // 将消息序列化为字符串
            std::string msgStr = msg.dump();
            size_t msgLen = msgStr.size();

            // 分发给每个消费者（通过线程池异步发送）
            for (SOCKET client : consumers) {
                threadPool_->enqueue([client, msgStr, msgLen]() {
                    if (send(client, msgStr.c_str(), static_cast<int>(msgLen), 0)!=msgLen)
                    {
                        LOG_ERROR("Failed to send message to client, socket");
                    }
                });
            }
        } catch (const std::exception& e) {
            // LOG_ERROR("Failed to dispatch message: {}");
             std::cout<< e.what() << std::endl;
        }
    }

public:
    MessageDispatcher(const std::shared_ptr<ConsumerManager> &consumerManager,
                     const std::shared_ptr<ThreadPool> &threadPool,
                     const std::shared_ptr<Queue<json>> &msgQueue)
        : consumerManager_(consumerManager),
          threadPool_(threadPool),
            msgQueue_(msgQueue){}

    // 启动分发器
    void start() {
        if (!isRunning_) {
            isRunning_ = true;
            // 在线程池中启动分发循环
            threadPool_->enqueue([this]() { dispatchLoop(); });
        }
    }

    // 停止分发器
    void stop() {
        isRunning_ = false;
    }

};
