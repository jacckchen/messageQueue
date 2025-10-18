#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <queue>
#include <thread>
#include <random>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include "json.hpp"
#include "time_stamp.h"
#include "InitClientSocket.h"
#include "net/TcpClient.h"

const char *IP="127.0.0.1";
using namespace std;
using json = nlohmann::json;
std::mutex mtx;
std::condition_variable cv;
atomic<int> messageCount;
static int cnt = 100000;
ofstream ofs;

json create_message(const int &content) {
    json message;
    //message["message_id"] = (int)messageCount;
    messageCount++;
    message["timestamp"] = get_iso8601_timestamp();
    message["type"] = "Produce";
    message["tag"] = "RandomInt";
    message["body"] = {{"RandomInt", content}};
    return message;
}

class Queue {
private:
    queue<int> buffer;
public:
    int deQueue() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            int x = buffer.front();
//            cout<<"get x"<<x;
            this->buffer.pop();
            return x;
        }
    }

    void enQueue(int x) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            this->buffer.push(x);
        }
        cv.notify_one();
    }

    bool Empty() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            return this->buffer.empty();
        }
    }
};

class Producer {
public:
    int num;

    Producer() = default;
    void produce(Queue *buffer) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(2000000000, INT_MAX);
        while (cnt > 0) {
            this->num = dist(gen);
//            cout<<"generate"<< this->num;
            buffer->enQueue(this->num);
            cnt--;
//            cout<<"enqueued"<< this->num;
        }
    }
//    bool enqueue(std::queue<int> duilie);
};

int newProducer(Queue *buffer) {
    Producer producer{};
    producer.produce(buffer);
//    cout<<"produce initialized"<<endl;
    return 0;
}
int sender(Queue *buffer) {
    TcpClient tcpClient;
    if (!tcpClient.connect(IP, 3001)) {
        return 1;
    }
    if (!tcpClient.isValid()) {
        return 1;
    }

    while (cnt > 0 || (!buffer->Empty())) {
        //std::this_thread::sleep_for(chrono::seconds(1));
        int value = buffer->deQueue();
        json data = create_message(value);
        const string s = data.dump()+"\n";

        if (tcpClient.send(s) == -1) {
            cout<<"send error"<<endl;
            return 1;
        }
        ofs<<s;
    }
    //看来我当时也想到了这种可能性,结果表明不是这个原因
//     std::this_thread::sleep_for(std::chrono::seconds(6));
//     json endData;
//     endData["type"] = "end";
//     const string endS = endData.dump() + "\n";
//     if (send(tcpClient.getSocket(), endS.c_str(), endS.size(), 0) == SOCKET_ERROR) {
//         cerr << "Error sending end message: " << WSAGetLastError() << endl;
//     }
//
// // 使用 shutdown 关闭发送通道
//     if (shutdown(tcpClient.getSocket(), SD_SEND) == SOCKET_ERROR) {
//         cerr << "Shutdown failed: " << WSAGetLastError() << endl;
//     }
//
// // 设置 SO_LINGER 选项以确保所有数据都被发送并且得到确认
//     linger ling{};
//     ling.l_onoff = 1;
//     ling.l_linger = 30; // 等待30秒
//     setsockopt(tcpClient.getSocket(), SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

    return 0;
}

int main() {
    Queue buffer;
    int NumberOfProducer = 1;
    for (int i = 1; i < NumberOfProducer; ++i) {
        thread produce(newProducer, &buffer);
        produce.detach();
    }
    ofs.open("./data.txt", std::ios::out);
    thread produce(newProducer, &buffer);
    produce.detach();
    thread send(sender, &buffer);
    send.join();
    ofs.close();
    return 0;
}
