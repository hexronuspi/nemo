#pragma once

#include "utils/types.h"
#include <string>
#include <map>
#include <fstream>
#include <chrono>
#include <iostream>
#include <sstream>
#include <ctime>
#include <mutex>
#include <memory>
#include <queue>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace backtest {

// Log levels
enum class LogLevel : uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5
};

// Log message structure
struct LogMessage {
    Timestamp timestamp;
    LogLevel level;
    std::string logger_name;
    std::string message;
    std::map<std::string, std::string> fields;
    
    LogMessage(LogLevel lvl, const std::string& name, const std::string& msg)
        : timestamp(std::chrono::high_resolution_clock::now()),
          level(lvl), logger_name(name), message(msg) {}
};

// High-performance async logger
class Logger {
public:
    static Logger& get();
    
    // Initialize logger with file path and settings
    void init(const std::string& filepath = "../logs/backtest.log", 
              bool remake = false, LogLevel min_level = LogLevel::INFO);
    
    // Log with structured fields
    void log(LogLevel level, const std::string& logger_name, 
             const std::string& message,
             const std::map<std::string, std::string>& fields = {});
    
    // Convenience methods
    void trace(const std::string& logger_name, const std::string& message, 
               const std::map<std::string, std::string>& fields = {}) {
        log(LogLevel::TRACE, logger_name, message, fields);
    }
    
    void debug(const std::string& logger_name, const std::string& message, 
               const std::map<std::string, std::string>& fields = {}) {
        log(LogLevel::DEBUG, logger_name, message, fields);
    }
    
    void info(const std::string& logger_name, const std::string& message, 
              const std::map<std::string, std::string>& fields = {}) {
        log(LogLevel::INFO, logger_name, message, fields);
    }
    
    void warn(const std::string& logger_name, const std::string& message, 
              const std::map<std::string, std::string>& fields = {}) {
        log(LogLevel::WARN, logger_name, message, fields);
    }
    
    void error(const std::string& logger_name, const std::string& message, 
               const std::map<std::string, std::string>& fields = {}) {
        log(LogLevel::ERROR, logger_name, message, fields);
    }
    
    void critical(const std::string& logger_name, const std::string& message, 
                  const std::map<std::string, std::string>& fields = {}) {
        log(LogLevel::CRITICAL, logger_name, message, fields);
    }
    
    // Log trading events
    void log_order(const Order& order);
    void log_fill(const Fill& fill);
    void log_signal(const std::string& strategy, const std::string& instrument, 
                   const std::string& signal_type, Price strength = 1.0);
    void log_market_data(const MarketDataTick& tick);
    void log_performance(const std::string& strategy, Price pnl, 
                        size_t trades, Price win_rate);
    
    // Start/stop async processing
    void start();
    void stop();
    void flush();
    
    // Set minimum log level
    void set_level(LogLevel level) { min_level_ = level; }
    
    // Legacy compatibility
    void log(const std::chrono::system_clock::time_point& record_time,
             const std::map<std::string, std::string>& fields);
    
private:
    Logger() = default;
    ~Logger() { stop(); }
    
    void process_messages();
    void write_message(const LogMessage& msg);
    std::string format_timestamp(Timestamp ts);
    std::string level_to_string(LogLevel level);
    void writeHeader();
    std::string timePointToString(const std::chrono::system_clock::time_point& tp);
    
    mutable std::mutex mutex_;
    std::ofstream file_;
    std::string current_path_;
    LogLevel min_level_ = LogLevel::INFO;
    bool header_written_ = false;
    uint64_t next_id_ = 1;
    
    // Legacy support
    std::ofstream _ofs;
    std::string _currentPath;
    bool _headerWritten = false;
    uint64_t _nextId = 1;
    
    // Async processing
    std::queue<LogMessage> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
};

} // namespace backtest
