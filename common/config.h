#pragma once
#include "../json.hpp"
#include <fstream>
using json = nlohmann::json;
class Config {
private:
    json data_;
public:
    Config(const std::string& path) noexcept{
        std::ifstream ifs(path);
        ifs >> data_;
    }
    std::string getServerIp() const { return data_["server"]["ip"]; }
    int getServerPort() const { return data_["server"]["port"]; }
    int getThreadPoolSize() const { return data_["server"]["thread_pool_size"]; }
    std::string getLogFilePath() const { return data_["server"]["log"]; }
};

