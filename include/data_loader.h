#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include <vector>
#include <string>
#include <map>

struct DataPoint {
    std::map<std::string, double> values; // Dynamic columns
};

class DataLoader {
public:
    // Loads CSV with dynamic columns, returns vector of DataPoint
    std::vector<DataPoint> load_data(const std::string& file_path);
};

#endif
