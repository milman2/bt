#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace bt {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        log_file_.open(filename, std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "로그 파일을 열 수 없습니다: " << filename << std::endl;
        }
    }

    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        log_level_ = level;
    }

    void log(LogLevel level, const std::string& message) {
        if (level < log_level_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();

        std::string level_str;
        switch (level) {
            case LogLevel::DEBUG: level_str = "DEBUG"; break;
            case LogLevel::INFO: level_str = "INFO"; break;
            case LogLevel::WARNING: level_str = "WARNING"; break;
            case LogLevel::ERROR: level_str = "ERROR"; break;
        }

        std::string log_message = "[" + ss.str() + "] [" + level_str + "] " + message;

        // 콘솔에 출력
        std::cout << log_message << std::endl;

        // 파일에 출력
        if (log_file_.is_open()) {
            log_file_ << log_message << std::endl;
            log_file_.flush();
        }
    }

    void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
    void info(const std::string& message) { log(LogLevel::INFO, message); }
    void warning(const std::string& message) { log(LogLevel::WARNING, message); }
    void error(const std::string& message) { log(LogLevel::ERROR, message); }

private:
    Logger() : log_level_(LogLevel::INFO) {}
    ~Logger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    std::ofstream log_file_;
    LogLevel log_level_;
    std::mutex mutex_;
};

// 편의 매크로
#define LOG_DEBUG(msg) bt::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) bt::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) bt::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) bt::Logger::getInstance().error(msg)

} // namespace bt
