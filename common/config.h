#pragma once
#include "../json.hpp"
#include <fstream>
using json = nlohmann::json;
class Config {
private:
    json data_;
public:
   explicit Config(const std::string& path) noexcept{
        std::ifstream ifs(path);
        ifs >> data_;
    }
    [[nodiscard]] std::string getServerIp() const { return data_["server"]["ip"]; }
    [[nodiscard]] uint16_t getServerPort() const { return data_["server"]["port"]; }
    [[nodiscard]] int getThreadPoolSize() const { return data_["server"]["thread_pool_size"]; }
    [[nodiscard]] std::string getLogFilePath() const { return data_["server"]["log"]; }
    [[nodiscard]] std::string getMode() const { return data_["server"]["mode"]; }
    [[nodiscard]] std::uint16_t getReceivePort() const { return data_["server"]["receive_port"]; }
};

