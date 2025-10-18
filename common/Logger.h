#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

// 日志级别：从低到高，可控制输出哪些日志
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};


// 日志工具类：线程安全，支持控制台+文件输出
class Logger {
private:
    // 单例模式：全局唯一日志实例
    static Logger instance_;

    // 成员变量
    std::ofstream log_file_;       // 日志文件流
    std::mutex mtx_;               // 互斥锁（保证线程安全）
    LogLevel current_level_;       // 当前日志级别（控制输出）
    std::string log_path_;         // 日志文件路径

    // 私有构造函数（单例模式）
    Logger() : current_level_(LogLevel::Debug), log_path_("./mq_log.txt") {
        // 初始化时打开日志文件（追加模式）
        openLogFile();
    }

    // 打开/创建日志文件
    void openLogFile() {
        // 以追加模式打开，不存在则创建
        log_file_.open(log_path_, std::ios::app | std::ios::out);
        if (!log_file_.is_open()) {
            // 若文件打开失败，至少保证控制台输出
            std::cerr << "日志文件打开失败：" << log_path_ << std::endl;
        }
    }

    // 获取当前时间的字符串（ISO8601格式：YYYY-MM-DD HH:MM:SS）
    static std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_now{};
        localtime_s(&tm_now, &time_t_now); // Windows用localtime_s，Linux用localtime_r

        std::stringstream ss;
        ss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // 日志级别转字符串
    static std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO";
            case LogLevel::Warning:  return "WARN";
            case LogLevel::Error: return "ERROR";
            default: return "UNKNOWN";
        }
    }

public:
    // 禁止拷贝（单例模式）
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // 获取全局唯一实例
    static Logger& getInstance() {
        return instance_;
    }

    // 设置日志级别（如发布时设为INFO，屏蔽DEBUG信息）
    void setLevel(LogLevel level) {
        current_level_ = level;
    }

    // 设置日志文件路径（默认./mq_log.txt）
    void setLogPath(const std::string& path) {
        log_path_ = path;
        // 重新打开文件
        if (log_file_.is_open()) {
            log_file_.close();
        }
        openLogFile();
    }

    // 核心日志输出函数
    void log(LogLevel level, const std::string& message) {
        // 若日志级别低于当前设置，不输出（如当前是INFO，DEBUG不输出）
        if (level < current_level_) {
            return;
        }

        // 加锁保证线程安全（多线程同时写日志不会错乱）
        std::lock_guard<std::mutex> lock(mtx_);

        // 构建日志内容：时间 [级别] 消息
        std::string log_line = "[" + getCurrentTime() + "] ["
                               + levelToString(level) + "] " + message + "\n";

        // 输出到控制台
        std::cout << log_line;

        // 输出到文件（若文件打开成功）
        if (log_file_.is_open()) {
            log_file_ << log_line;
            log_file_.flush(); // 立即写入，避免程序崩溃时丢失日志
        }
    }
};

// 便捷宏定义：简化日志调用（如 LOG_INFO("连接成功")）
#define LOG_DEBUG(msg) Logger::getInstance().log(LogLevel::Debug, msg)
#define LOG_INFO(msg)  Logger::getInstance().log(LogLevel::Info, msg)
#define LOG_WARN(msg)  Logger::getInstance().log(LogLevel::Warn, msg)
#define LOG_ERROR(msg) Logger::getInstance().log(LogLevel::Error, msg)