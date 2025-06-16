#pragma once

#include <string>
#include <map>
#include <fstream>
#include <chrono>
#include <iostream>
#include <sstream>
#include <ctime>

class Logger {
public:
    static Logger& get();

    void init(const std::string& filepath = "../logs/log.txt", bool remake = false);

    void log(const std::chrono::system_clock::time_point& record_time,
             const std::map<std::string, std::string>& fields);

private:
    Logger() = default;

    void writeHeader();
    std::string timePointToString(const std::chrono::system_clock::time_point& tp);

    std::ofstream _ofs;
    std::string _currentPath;
    bool _headerWritten = false;
    uint64_t _nextId = 1;
};
