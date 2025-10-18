//
// Created by 13717 on 2025/10/9.
//

#include "ClusterDispatcher.h"
void ClusterDispatcher::start() {
    isRunning_ = true;
    threadPool_->enqueue([this]() { dispatchLoop(); });
}
void ClusterDispatcher::dispatchLoop() {
    while (isRunning_) {
        json msg;
        if ((msg = msgQueue_->deQueue_front())!=nullptr) {
            dispatchMessage(msg);
        }else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}
// 在 ClusterDispatcher 中添加轮询状态管理
std::unordered_map<std::string, std::unordered_set<SOCKET>::iterator> topicIterators;

void ClusterDispatcher::dispatchMessage(const json& msg) const
{
    try {
        // 假设消息格式包含 topic 字段
        std::string topic = msg["tag"];

        // 从 ConsumerManager 获取订阅该话题的所有消费者
        auto consumers = consumerManager_->getConsumersByTopic(topic);
        if (consumers.empty()) {
            msgQueue_->enQueue_back(msg);
            return;
        }

        // 转换为 vector 以便使用索引访问
        const std::vector<SOCKET> consumerList(consumers.begin(), consumers.end());

        // 获取当前话题的轮询索引
        auto& index = topicRoundRobinIndex[topic];

        // 确保索引在有效范围内
        if (index >= consumerList.size()) {
            index = 0;
        }

        // 选择当前索引的消费者
        const SOCKET &selectedConsumer = consumerList[index];
        const std::string &&r=msg.dump()+"\n";
        send(selectedConsumer, r.c_str(), static_cast<int>(r.size()), 0);

        // 更新轮询索引
        index = (index + 1) % consumerList.size();

    } catch (const std::exception& e) {

    }
}

void ClusterDispatcher::stop() {
    isRunning_ = false;
}