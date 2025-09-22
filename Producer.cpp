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
            unique_lock<mutex> lock(mtx);
            int x = buffer.front();
//            cout<<"get x"<<x;
            this->buffer.pop();
            return x;
        }
    }

    void enQueue(int x) {
        {
            unique_lock<mutex> lock(mtx);
            this->buffer.push(x);
        }
        cv.notify_one();
    }

    bool Empty() {
        {
            unique_lock<mutex> lock(mtx);
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
    SOCKET serverSocket;
    if (InitClientSocket(serverSocket,IP,3000) != 0) {
        return 1;
    }
    while (cnt > 0 || (!buffer->Empty())) {
        //std::this_thread::sleep_for(chrono::seconds(1));
        int value = buffer->deQueue();
        json data = create_message(value);
        const string s = data.dump()+"\n";

        //cout<<"发送"<<s.c_str()<<endl;
        if (send(serverSocket, s.c_str(), strlen(s.c_str()), 0) == SOCKET_ERROR) {
            // 错误处理逻辑
            cerr << "Error sending data: " << WSAGetLastError() << endl;
            break; // 或者采取其他适当的行动
        }
        ofs<<s;
    }
    //看来我当时也想到了这种可能性,结果表明不是这个原因
    std::this_thread::sleep_for(std::chrono::seconds(6));
    json endData;
    endData["type"] = "end";
    const string endS = endData.dump() + "\n";
    if (send(serverSocket, endS.c_str(), endS.size(), 0) == SOCKET_ERROR) {
        cerr << "Error sending end message: " << WSAGetLastError() << endl;
    }

// 使用 shutdown 关闭发送通道
    if (shutdown(serverSocket, SD_SEND) == SOCKET_ERROR) {
        cerr << "Shutdown failed: " << WSAGetLastError() << endl;
    }

// 设置 SO_LINGER 选项以确保所有数据都被发送并且得到确认
    linger ling{};
    ling.l_onoff = 1;
    ling.l_linger = 30; // 等待30秒
    setsockopt(serverSocket, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

    closesocket(serverSocket);
    WSACleanup();
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
    //    std::thread receivethread(mythread,&buffer);
    //    receivethread.detach();
    thread produce(newProducer, &buffer);
    produce.detach();
    thread send(sender, &buffer);
    send.join();
    ofs.close();
    return 0;
}
