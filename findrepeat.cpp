//
// Created by 13717 on 2025/4/17.
//
#include <iostream>
#include <fstream>
#include "json.hpp"
using  namespace std;
using json=nlohmann::json;
bool set[1000000000];
int main()
{
    ifstream ifs;
    ifs.open("./log.txt", std::ios::in);
    //读取数据入队
    if (!ifs.is_open()) {
        std::cerr << "Failed to open file." << std::endl;
        return -1;
    }
    int cnt=0;
    std::string line;
    //启动时将文件中的数据入队
    while (std::getline(ifs, line)) {
        try {
            // 解析 JSON
            line=line.substr(13, line.size()-13);
            auto j = json::parse(line);
            int id=j["body"]["RandomInt"];

            if(!set[id-2000000000]) {
                set[id-2000000000] = true;
            } else{
                cnt++;
                cout<<id<<endl;
            }
            // 提取时间戳字符串
        } catch (json::exception &e) {
            std::cerr << "JSON parsing error for line: " << line
                      << ", error: " << e.what() << std::endl;
            continue; // 继续处理下一行
        }
    }
    cout<<cnt<<endl;
    ifs.close();
//    ifs.open("./log.txt", std::ios::in);
//    //读取数据入队
//    if (!ifs.is_open()) {
//        std::cerr << "Failed to open file." << std::endl;
//        return -1;
//    }
//    memset(set,0,sizeof(set));
//    cnt=0;
//    //启动时将文件中的数据入队
//    while (std::getline(ifs, line)) {
//        unsigned long long int n;
//        n= stoll(line);
//        if(!set[n]) {
//            set[n] = true;
//        } else{
//            cnt++;
//            cout<<n<<endl;
//        }
//    }
//    ifs.close();
//    cout<<cnt<<endl;
}
