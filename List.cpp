#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <fstream>
#include <codecvt>
#include "ThreadPool.h"

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "json.hpp"
#include "Queue.h"
#include "common/config.h"

const char *IP = "127.0.0.1";
using json = nlohmann::json;
using namespace std;
ifstream ifs;
ofstream ofs;
ofstream logtxt;
ofstream logtxt2;
atomic<int> messageCount = -1;
int numberOfConsumer = 0;
std::string mode;
ThreadPool pool(50);
std::mutex messageIdmtx;

Queue<pair<SOCKET, string>> clientAndTopic;

void heartbeat(SOCKET clientSocket, bool *closed) {
    char recvBuffer[1024];
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(6));
        if (recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0) <= 0) {
            closesocket(clientSocket);
            *closed = true;
            cout << "未检测到心跳" << endl;
            break;
        }
        //cout<<"检测到心跳"<<endl;
    }
}

//集群模式下发送消息的函数
void mySend(Queue<json> *buffer) {
    if(mode=="jq")
    {
        list<pair<SOCKET, string>>::iterator it;
        if (clientAndTopic.Empty()) {
            unique_lock<std::mutex> lock(clientAndTopic.mtx2);
            clientAndTopic.cv.wait(lock);
        }
        it = clientAndTopic.theList.begin();
        while (true) {
            if (buffer->Empty()) {
                unique_lock<std::mutex> lock(buffer->mtx2);
                buffer->cv.wait(lock, [&buffer] { return !buffer->Empty(); });
            }
            if (clientAndTopic.Empty()) {
                unique_lock<std::mutex> lock(clientAndTopic.mtx2);
                clientAndTopic.cv.wait(lock);
            }
            json j = buffer->deQueue_front();
            const string s = j.dump()+'\n';
            //cout << (*it).second << ":" << (*it).first << endl;
            //cout << "进行判断" << endl;
            //cout << j["tag"] << ":" << (*it).second << endl;
//        std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
            resend:
            if (j["tag"] == (*it).second) {
                cout << "发送：" << s ;
                send((*it).first, s.c_str(), (int) s.size(), 0);
            }
            else{
                it++;
                goto resend;
            }
            it++;
            if (it == clientAndTopic.theList.end())
                it = clientAndTopic.theList.begin();
        }
    }
}

//接收来自生产者的消息
void handleReceiveInt(Queue<json> *buffer,const string *s) {
    static atomic<int> start=0;
    start++;
//    cout<<"接收函数启动"<<start<<endl;
    stringstream ss(*s);
    logtxt<<s<<*s;
    string oneline;
    while (getline(ss, oneline, '\n')) {
        json j = json::parse(oneline);
        if (j["type"] == "Produce") {
            {
                std::lock_guard<std::mutex> lock(messageIdmtx);
                if (messageCount != -1)
                {
                    messageCount++;
                }
                else
                {
                    messageCount = 0;
                }
                j["message_id"] = (int)messageCount;
                buffer->enQueue_back(j);
            }
            ofs << j.dump() << "\n";
//            cout<<"接收到"<<j.dump()<<endl;
            //if (mode == "jq") ofs << s << endl;
            continue;
        }
        if (j["type"] == "end") {
            cout<<"接收到end,退出接收函数"<<endl;
            break;
        }
    }
    delete s;
}

void handle(SOCKET &clientSocket, Queue<json> *buffer) {
    fd_set readfds;
    string sock = "unknown";
    struct timeval timeout{};
    //缓冲区要调偏大，否则会出现select结果为0，然后关闭连接的错误，而且WSA返回值为0
    char recvbuffer[1080000];
    FD_ZERO(&readfds);
    FD_SET(clientSocket, &readfds);
    timeout.tv_sec = 10; // 设置超时时间为10秒
    timeout.tv_usec = 0;
    while (true) {
        int selectResult = select(clientSocket + 1, &readfds, nullptr, nullptr,
                                  &timeout);
        if (selectResult > 0) {
//            if(FD_ISSET(STDIN_FILENO, &readfds))
//            {
//                if()
//                closesocket(clientSocket);
//                WSACleanup();
//            }
            if (FD_ISSET(clientSocket, &readfds)) {
                bool clientClosed = false;
                int bytesReceived = recv(clientSocket, recvbuffer, sizeof(recvbuffer), 0);
                if (bytesReceived > 0) {
                    recvbuffer[bytesReceived] = '\0';
                    //logtxt<<"进行转为字符串";
                    string *s=new string(recvbuffer);
                    memset(recvbuffer, 0, bytesReceived);
//                    ofs<<s;
//                    logtxt<< s;
                    if (sock == "Produce") {
                        logtxt2<<"enqueue"<<s;
                        pool.enqueue(handleReceiveInt, buffer, s);
                    }
                    if (sock == "unknown") {
                        stringstream ss(*s);
                        string oneLine;
                        getline(ss, oneLine, '\n');
                        json j = json::parse(oneLine);
                        if (j["type"] == "Produce")
                        {
                            sock = "Produce";
                            logtxt2<<"enqueue"<<s;
                            pool.enqueue(handleReceiveInt, buffer, s);
                        }
                        else if (j["type"] == "Register") {
                            sock = "Consume";
                        }
                    }

                    if (sock == "Consume") {
                        json j = json::parse(*s);
                        if (j["type"] == "Register") {
                            if (j["mode"] != mode) {
                                cout << j["mode"] << ":" << mode << endl;
                                *s = "error,mode unmatched";
                                send(clientSocket, (*s).c_str(), (int) (*s).size(), 0);
                                closesocket(clientSocket);
                                break;
                            }
                            clientAndTopic.enQueue_back(pair{clientSocket, j["body"]["RequireTag"]});
                            numberOfConsumer++;
                            if (mode == "jq") {
                                while (true) {
                                    std::this_thread::sleep_for(std::chrono::seconds(35));
                                    if (recv(clientSocket, recvbuffer, sizeof(recvbuffer), 0) <= 0) {
                                        for (auto it = clientAndTopic.theList.begin();
                                             it != clientAndTopic.theList.end(); it++) {
                                            if ((*it).first == clientSocket)
                                                clientAndTopic.Erase(it);
                                        }
                                        closesocket(clientSocket);
                                        clientClosed = true;
                                        break;
                                    }
                                }
                            }
                            if (mode == "gb") {
                                std::thread beat(heartbeat, clientSocket, &clientClosed);
                                beat.detach();
                                int id = (int) j["id"];
                                ofstream sended;
                                sended.open("./send.txt", std::ios::out);
                                for (auto it = buffer->theList.begin();; it++) {
//                                if((*it).dump()=="null")
//                                    it=buffer->buffer.begin();
                                    if (id == (int) (*it)["message_id"]) {
                                        for (; it != buffer->theList.end(); it++) {
//                                            std::this_thread::sleep_for(std::chrono::nanoseconds (1));
                                            j = *it;
                                            *s = j.dump()+'\n';
                                            if (*s == "null")
                                                break;
                                            //cout << "send:" << s << endl;
                                            send(clientSocket, (*s).c_str(), strlen((*s).c_str()), 0);
                                            sended<<s<<endl;
                                            id++;
                                        }
                                    }
                                    if (clientClosed) {
                                        cout << "正在移除客户端·" << endl;
                                        for (auto it2 = clientAndTopic.theList.begin();
                                             it2 != clientAndTopic.theList.end(); it2++) {
                                            if ((*it2).first == clientSocket)
                                                clientAndTopic.Erase(it2);
                                        }
                                        closesocket(clientSocket);
                                        clientClosed = true;
                                        sended.close();
                                        cout << "移除完成，跳出主循环" << endl;
                                        break;
                                    }
                                    if(it==buffer->theList.end())
                                        it=buffer->theList.begin();
                                }
                            }
                        }
                        if (clientClosed)
                            break;
                    }
                } else if (bytesReceived == 0) {
                    std::cout << "对端已关闭端口： " << WSAGetLastError() << std::endl;
                    closesocket(clientSocket);
                    return;
                } else {
                    std::cerr << "接收数据失败，错误代码: " << WSAGetLastError() << std::endl;
                    closesocket(clientSocket);
                    return;
                }
            }

        } else if (selectResult == 0) {
            std::cerr << "连接超时"<<WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            return;
        } else {
            std::cerr << "select失败，错误代码: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            return;
        }
    }

}
int initSocket(SOCKET &serverSocket)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock初始化失败，错误代码: " << WSAGetLastError() << std::endl;
        return 1;
    }
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket创建失败，错误代码: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(IP);
    serverAddr.sin_port = htons(3000);
    int optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char *) &optval, sizeof(optval)) == SOCKET_ERROR) {
        std::cerr << "设置socket选项失败，错误代码: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    if (bind(serverSocket, (sockaddr *) &serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "绑定失败，错误代码: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    if (listen(serverSocket, 1024) == SOCKET_ERROR) {
        std::cerr << "监听失败，错误代码: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    return 0;
}
int mythread(Queue<json> *buffer) {
    SOCKET serverSocket;
    if(initSocket(serverSocket)!=0)
    {
        return 1;
    }
    if (mode == "jq")
        pool.enqueue(mySend, buffer);
    while (true) {
        std::cout << "等待连接...\n";
        SOCKET clientSocket;
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (sockaddr *) &clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "接受连接失败，错误代码:" << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            return 1;
        }
        pool.enqueue(handle, clientSocket, buffer);
    }
}

int main() {
    int choice;
    Config config("../config/mq_config.json");
    mode=config.getMode();
    Queue<json> buffer;
    logtxt.open("./log.txt", std::ios::out);
    logtxt2.open("./log2.txt", std::ios::out);
    if (mode == "gb") {
        ifs.open("./gb.txt", std::ios::in);
        //读取数据入队
        if (!ifs.is_open()) {
            std::cerr << "Failed to open file." << std::endl;
            return -1;
        }
        std::string line;
        //启动时将文件中的数据入队
        while (std::getline(ifs, line,'\n')) {
            //等待完善
            if(line.empty())continue;
            try {
                // 解析 JSON
                auto j = json::parse(line);
                // 提取时间戳字符串
                buffer.enQueue_back(j);
            } catch (json::exception &e) {
                std::cerr << "JSON parsing error for line: " << line
                          << ", error: " << e.what() << std::endl;
                continue; // 继续处理下一行
            }
        }
        if(!buffer.Empty())
            messageCount = (int) buffer.theList.back()["message_id"] + 1;
        else  messageCount=0;
        ifs.close();
        ofs.open("./gb.txt", std::ios::out);
        //    std::thread receivethread(mythread,&buffer);
        //    receivethread.detach();
        std::thread thethread(mythread, &buffer);
        thethread.join();
        //关闭时存回数据
        while (!buffer.Empty()) {
            json j = buffer.deQueue_front();
            std::string timestamp_str = j["timestamp"];
            // 解析时间戳
            // 创建一个 istringstream 对象
            std::istringstream ss(timestamp_str);
            // 定义 tm 结构体来存储解析后的日期时间信息
            std::tm tm = {};
            // 使用 iomanip 操纵器来指定输入格式，并尝试解析日期时间字符串
            ss >> std::get_time(&tm, "%Y-%m-%d-%H:%M:%S+0800");
            if (ss.fail()) {
                std::cerr << "Parsing failed.\n";
                return -1;
            }

            auto parsed_tp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            // 获取当前时间点
            auto now_tp = std::chrono::system_clock::now();
            // 计算两个时间点之间的差异（持续时间）
            auto duration = now_tp - parsed_tp;
            //两个时间点之间的差异
            //12个小时的秒数：43,200
            if (duration_cast<std::chrono::seconds>(duration).count() < 43200)
                ofs << j.dump() << endl;
        }
        ofs.close();
    }
    if (mode == "jq") {
        ofs.open("./jq.txt", std::ios::out);
        std::thread thethread(mythread, &buffer);
        thethread.join();
        ofs.close();
    }
    if(mode == "config")
    {

    }
    WSACleanup();
    return 0;
}
