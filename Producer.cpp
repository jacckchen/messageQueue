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

using namespace std;
using json = nlohmann::json;
std::mutex mtx;
std::condition_variable cv;
atomic<int> messageCount;
int cnt = 10000;
ofstream ofs;
std::string get_iso8601_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d-%H:%M:%S");
    return ss.str();
}

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
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock初始化失败，错误代码: " << WSAGetLastError() << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket创建失败，错误代码: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(3000);

    struct timeval timeout{};
    timeout.tv_sec = 5;  // 秒
    timeout.tv_usec = 0; // 微秒
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *) &timeout, sizeof(timeout));
    if (connect(clientSocket, (sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "连接失败，错误代码: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    while (cnt > 0 || (!buffer->Empty())) {
        //std::this_thread::sleep_for(chrono::seconds(1));
        int value = buffer->deQueue();
        json data = create_message(value);
        const string s = data.dump()+"\n";

        //cout<<"发送"<<s.c_str()<<endl;
        if (send(clientSocket, s.c_str(), strlen(s.c_str()), 0) == SOCKET_ERROR) {
            // 错误处理逻辑
            cerr << "Error sending data: " << WSAGetLastError() << endl;
            break; // 或者采取其他适当的行动
        }
        ofs<<s;
    }
    std::this_thread::sleep_for(std::chrono::seconds(6));
    json endData;
    endData["type"] = "end";
    const string endS = endData.dump() + "\n";
    if (send(clientSocket, endS.c_str(), endS.size(), 0) == SOCKET_ERROR) {
        cerr << "Error sending end message: " << WSAGetLastError() << endl;
    }

// 使用 shutdown 关闭发送通道
    if (shutdown(clientSocket, SD_SEND) == SOCKET_ERROR) {
        cerr << "Shutdown failed: " << WSAGetLastError() << endl;
    }

// 设置 SO_LINGER 选项以确保所有数据都被发送并且得到确认
    linger ling{};
    ling.l_onoff = 1;
    ling.l_linger = 30; // 等待30秒
    setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

    closesocket(clientSocket);
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
