#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include <vector>
#include <string>

struct DataPoint {
    std::string timestamp;
    double open;
    double high;
    double low;
    double close;
};

class DataLoader {
public:
    std::vector<DataPoint> load_data(const std::string& file_path);
};

#endif
