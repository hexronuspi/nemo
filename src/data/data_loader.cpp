#include "data_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::vector<DataPoint> DataLoader::load_data(const std::string& file_path) {
    std::vector<DataPoint> data;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << "\n";
        return data;
    }

    std::string line;
    bool header_skipped = false;

    while (std::getline(file, line)) {
        if (!header_skipped) {
            header_skipped = true;
            continue; 
        }

        std::istringstream iss(line);
        std::string token;
        DataPoint dp;

        std::getline(iss, dp.timestamp, ','); 
        std::getline(iss, token, ','); dp.open = std::stod(token);
        std::getline(iss, token, ','); dp.high = std::stod(token);
        std::getline(iss, token, ','); dp.low  = std::stod(token);
        std::getline(iss, token, ','); dp.close = std::stod(token);
        std::getline(iss, token, ','); dp.volume = std::stod(token);
        std::getline(iss, token, ','); dp.oi = std::stod(token);

        data.push_back(dp);
    }

    file.close();
    return data;
}
