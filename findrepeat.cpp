//
// Created by 13717 on 2025/4/17.
//
#include <iostream>
#include <fstream>
#include "json.hpp"
using  namespace std;
using json=nlohmann::json;
bool set[100005];
int main()
{
    ifstream ifs;
    ifs.open("./gb.txt", std::ios::in);
    //读取数据入队
    if (!ifs.is_open()) {
        std::cerr << "Failed to open file." << std::endl;
        return -1;
    }
    std::string line;
    //启动时将文件中的数据入队
    while (std::getline(ifs, line)) {
        try {
            // 解析 JSON
            auto j = json::parse(line);
            int id=j["message_id"];

            if(!set[id]) {
                set[id] = true;
            } else{
                cout<<id<<endl;
            }
            // 提取时间戳字符串
        } catch (json::exception &e) {
            std::cerr << "JSON parsing error for line: " << line
                      << ", error: " << e.what() << std::endl;
            continue; // 继续处理下一行
        }
    }
}
