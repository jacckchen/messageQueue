
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <queue>
#include <thread>
#include <atomic>
#include <fstream>
#include "json.hpp"
#include <mutex>
#include "time_stamp.h"
#include "InitClientSocket.h"
using namespace std;
using json =nlohmann::json;
string mode;
ofstream outfile;
ofstream ofs;
int messageID=1;
const char *IP="127.0.0.1";
template<class T>
json create_message(const std::string &type, const T& content,const string &themode,const int &id) {
    json message;
    message["timestamp"] = get_iso8601_timestamp();
    message["type"]=type;
    if(type=="Register")
    {
        message["mode"]=themode;
        message["id"]=id;
        message["body"] = {
                {"RequireTag", content},
                {"IP",string(IP)},
        };
    }
    else if(type=="Heartbeat")
    {

    }
    return message;
}
class Consumer {
public:
    static std::atomic<int> n;
    bool socketClosed= true;
    int num=0;
    Consumer() = default;
    int consume();
    static int handleReceive(string *s);
    static bool IsPrime(int x);
    static void heartbeat(SOCKET serverSocket,const bool *socketClosed);
//    static std::queue<int>* queue;
};
int Consumer::consume() {
    bool prime;
    SOCKET serverSocket;
    if((InitClientSocket(serverSocket,IP,3000)!=0))
    {
        return 1;
    }
    socketClosed=false;
    json j= create_message("Register","RandomInt",mode,messageID);
    const string r=j.dump();
    send(serverSocket, r.c_str(), strlen(r.c_str()), 0);
    std::thread heart(heartbeat,serverSocket,&(this->socketClosed));
    heart.detach();
//            outfile<<"after compute:"<<Consumer::n<<" ";
    char recvbuffer[1080000];
    while(true)
    {
        std::this_thread::sleep_for(chrono::nanoseconds(1));
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        struct timeval timeout{};
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int selectResult = select(serverSocket + 1, &readfds, nullptr, nullptr, &timeout);
        if(selectResult>0)
        {
            if(FD_ISSET(serverSocket, &readfds)){
                int bytesReceived = recv(serverSocket, recvbuffer, sizeof(recvbuffer), 0);
                if (bytesReceived > 0) {
                    recvbuffer[bytesReceived]='\0';
                    string recvs=recvbuffer;
                    ofs<<recvs;
                    std::string_view subview(recvs.c_str(),5);
                    if(subview=="error")
                    {
                        std::this_thread::sleep_for(chrono::milliseconds(1000));
                        continue;
                    }
                    auto *s=new string(recvs);
                    std::thread t(handleReceive, s);
                    t.detach();

                }
                else if(bytesReceived==0)
                {
                    socketClosed=true;
                    cout<<"该连接已关闭"<<endl;
                    break;
                }else if(bytesReceived<0)
                {
                    cout<<"socket closed, error"<<WSAGetLastError();
                    break;
                }
            }
        } else if (selectResult == 0) {
            continue;
        } else {
            std::cerr << "select失败，错误代码: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            return 1;
        }
    }
    return 0;
}
bool Consumer::IsPrime(int x) {
    int i;
    if (x < 2 || (x != 2 && x % 2 == 0))//this->num小于2或者this->num是不等于2的偶数，必然非素数
        return false;
    else//这里n都是奇数
    {//这里使用上面刚提到的写法，用i代替开根号的过程
        for (i = 3; i * i <= x; i += 2)//这里注意循环条件
        {//2必然不是因子，从3开始，每次递增2，直到sqrt(x)为止
            if (x % i == 0)
                return false;
        }
        return true;
    }
}
void Consumer::heartbeat(SOCKET serverSocket,const bool *socketClosed)
{
    while(true)
    {

        if(*socketClosed)break;
        json j= create_message("Heartbeat","","",messageID);
        const string s=j.dump();
        cout<<"发送了一次心跳"<<endl;
        send(serverSocket, s.c_str(), strlen(s.c_str()), 0);
        std::this_thread::sleep_for(chrono::seconds(5));
    }
}

int Consumer::handleReceive(string *s) {
    stringstream ss(*s);
    string oneline;
    while (getline(ss, oneline, '\n')) {
        try{
            json data=json::parse(oneline);
            bool prime = IsPrime(data["body"]["RandomInt"]);
            outfile<<data["body"]["RandomInt"]<<':';
            if (prime) {
                outfile<<"1";
            } else {
                outfile<<"0";
            }
            outfile << endl;
        } catch (const std::exception& e) {
            cerr << "解析JSON时发生错误: " << e.what() << endl;
        }
    }
    delete s;
    return 0;
}

int main() {
    int choice;
    cout<<"请选择消费模式："<<endl<<"1,广播消费 2,集群消费"<<endl;
    cin>>choice;
    if(choice==1)
    {
        mode="gb";
    } else if(choice==2)
    {
        mode="jq";
    } else
    {
        cout<<"错误选择";
        return 0;
    }
    outfile.open("./result.txt",std::ios::out);
    ofs.open("./consumerReceive.txt",std::ios::out);
    Consumer consumer{};
    consumer.consume();
    outfile.close();
    ofs.close();
    return 0;
}
