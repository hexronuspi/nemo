#include "data_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>

std::vector<DataPoint> DataLoader::load_data(const std::string& file_path) {
    std::vector<DataPoint> data;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << "\n";
        return data;
    }

    std::string line;
    std::vector<std::string> headers;
    if (std::getline(file, line)) {
        std::istringstream header_stream(line);
        std::string col;
        while (std::getline(header_stream, col, ',')) {
            headers.push_back(col);
        }
    }

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        DataPoint dp;
        size_t col_idx = 0;
        while (std::getline(iss, token, ',')) {
            if (col_idx < headers.size()) {
                try {
                    dp.values[headers[col_idx]] = std::stod(token);
                } catch (...) {
                    // If not a double, skip or store as 0
                    dp.values[headers[col_idx]] = 0.0;
                }
            }
            ++col_idx;
        }
        data.push_back(dp);
    }

    file.close();
    return data;
}
