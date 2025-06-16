#include "logging.h"

#ifdef _WIN32
    #include <direct.h> 
    #define mkdir(dir, mode) _mkdir(dir)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <cerrno>
    #include <cstring>
#endif

#include <sys/stat.h> 

#include <iomanip>
#include <string.h>

Logger& Logger::get() {
    static Logger instance;
    return instance;
}

void Logger::init(const std::string& filepath, bool remake) {
    _headerWritten = false;
    _nextId = 1;
    _currentPath = filepath;

    size_t slash_pos = filepath.find_last_of("/\\");
    if (slash_pos != std::string::npos) {
        std::string dir = filepath.substr(0, slash_pos);

        struct stat st;
        if (stat(dir.c_str(), &st) != 0) {
            if (mkdir(dir.c_str(), 0777) != 0) {
                std::cerr << "❌ Failed to create directory: " << dir
                          << " (" << strerror(errno) << ")\n";
            }
        }
    }

    if (remake) {
        auto now_t = std::time(nullptr);
        std::tm tm = *std::localtime(&now_t);
        std::ostringstream oss;

        std::string dir = (slash_pos == std::string::npos) ? "" : filepath.substr(0, slash_pos);
        std::string base = (slash_pos == std::string::npos) ? filepath : filepath.substr(slash_pos + 1);
        auto dot = base.rfind('.');
        std::string name = (dot == std::string::npos) ? base : base.substr(0, dot);
        std::string ext  = (dot == std::string::npos) ? "" : base.substr(dot);

        oss << dir << "/" << name << "_"
            << std::put_time(&tm, "%Y%m%d_%H%M%S") << ext;
        _currentPath = oss.str();
        _ofs.open(_currentPath, std::ios::out | std::ios::trunc);
    } else {
        _ofs.open(_currentPath, std::ios::out | std::ios::app);
    }
}

void Logger::log(const std::chrono::system_clock::time_point& record_time,
                 const std::map<std::string, std::string>& fields) {
    if (!_ofs.is_open()) init(_currentPath, false);
    if (!_headerWritten) writeHeader();

    _ofs << _nextId++ << '\t';
    _ofs << timePointToString(std::chrono::system_clock::now()) << '\t';
    _ofs << timePointToString(record_time) << '\t';

    bool first = true;
    for (const auto& kv : fields) {
        if (!first) _ofs << '\t'; first = false;
        _ofs << kv.first << '=' << kv.second;
    }
    _ofs << '\n';
    _ofs.flush();
}

void Logger::writeHeader() {
    _ofs << "ID\tExecTime\tRecordTime\tFields\n";
    _headerWritten = true;
}

std::string Logger::timePointToString(const std::chrono::system_clock::time_point& tp) {
    auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}
