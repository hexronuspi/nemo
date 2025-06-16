#include "algo/simple_moving_average.h"
#include <vector>

SimpleMovingAverage::SimpleMovingAverage(int short_w, int long_w)
    : short_window(short_w), long_window(long_w) {}

std::vector<double> calculate_moving_average(const std::vector<DataPoint>& data, int window) {
    std::vector<double> ma(data.size(), 0.0);
    double sum = 0.0;
    for (size_t i = 0; i < data.size(); ++i) {
        sum += data[i].close;
        if (i >= window) {
            sum -= data[i - window].close;
            ma[i] = sum / window;
        } else if (i >= 1) {
            ma[i] = sum / (i + 1);
        } else {
            ma[i] = data[i].close;
        }
    }
    return ma;
}

std::vector<int> SimpleMovingAverage::generate_signals(const std::vector<DataPoint>& data) {
    std::vector<double> short_ma = calculate_moving_average(data, short_window);
    std::vector<double> long_ma = calculate_moving_average(data, long_window);
    std::vector<int> signals(data.size(), 0);
    for (size_t i = 1; i < data.size(); ++i) {
        if (short_ma[i] > long_ma[i] && short_ma[i-1] <= long_ma[i-1]) {
            signals[i] = 1;
        } else if (short_ma[i] < long_ma[i] && short_ma[i-1] >= long_ma[i-1]) {
            signals[i] = -1;  
        }
    }
    return signals;
}