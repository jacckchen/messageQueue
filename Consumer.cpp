
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <queue>
#include <thread>
#include <atomic>
#include <fstream>
#include "json.hpp"
#include <mutex>
using namespace std;
using json =nlohmann::json;
string mode;
ofstream outfile;
ofstream ofs;
int messageID=0;
const string IP="127.0.0.1";
std::string get_iso8601_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d-%H:%M:%S");
    return ss.str();
}
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
                {"IP",IP},
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
    int consume() {
        bool prime;

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

//            struct timeval timeout{};
//            timeout.tv_sec = 5;  // 秒
//            timeout.tv_usec = 0; // 微秒
//            setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
            if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                std::cerr << "连接失败，错误代码: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                WSACleanup();
                return 1;
            }
            socketClosed=false;
            json j= create_message("Register","RandomInt",mode,messageID);
            const string r=j.dump();
            send(clientSocket, r.c_str(), strlen(r.c_str()), 0);
            std::thread heart(heartbeat,clientSocket,&(this->socketClosed));
            heart.detach();
//            outfile<<"after compute:"<<Consumer::n<<" ";
            char recvbuffer[10800];
            while(true)
            {
                std::this_thread::sleep_for(chrono::nanoseconds(1));
                cout<<"正在接受"<<endl;
                int bytesReceived = recv(clientSocket, recvbuffer, sizeof(recvbuffer), 0);
                if (bytesReceived > 0) {
                    recvbuffer[bytesReceived]='\0';
                    string recvs=recvbuffer;
                    cout<<"收到"<<recvs<<endl;
                    ofs<<recvs<<endl;
                    if(recvs.substr(0,5)=="error")
                    {
                        std::this_thread::sleep_for(chrono::milliseconds(1000));
                        continue;
                    }
                    try{
                        json data=json::parse(recvs);
                        this->num=data["body"]["RandomInt"];
                        prime = IsPrime(this->num);
                        outfile<<std::to_string(this->num)<<':';
                        if (prime) {
                            outfile<<"1";
                        } else {
                            outfile<<"0";
                        }
                        outfile << endl;
                        continue;
                    } catch (const std::exception& e) {
                        cerr << "解析JSON时发生错误: " << e.what() << endl;
                    }
                }
                else if(bytesReceived==0)
                {
                    socketClosed=true;
                    cout<<"该连接已关闭"<<endl;
                    break;
                }else if(bytesReceived<0)
                {
                    cout<<"error"<<WSAGetLastError();
                    break;
                }
            }
        return 0;
    }
    static bool IsPrime(int x) {
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
    static void heartbeat(SOCKET clientSocket,const bool *socketClosed)
    {
        while(true)
        {
            std::this_thread::sleep_for(chrono::seconds(5));
            if(*socketClosed)break;
            json j= create_message("Heartbeat","","",messageID);
            const string s=j.dump();
            //cout<<"发送了一次心跳"<<endl;
            send(clientSocket, s.c_str(), strlen(s.c_str()), 0);
        }
    }
//    static std::queue<int>* queue;
};
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
    outfile.open("./id.txt",std::ios::out);
    ofs.open("./output.txt",std::ios::out);
    Consumer consumer{};
    consumer.consume();
    outfile.close();
    ofs.close();
    return 0;
}
