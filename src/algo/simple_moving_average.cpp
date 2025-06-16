#include "algo/simple_moving_average.h"
#include <vector>
#include <numeric> // For std::accumulate
#include <stdexcept> // For std::invalid_argument

SimpleMovingAverage::SimpleMovingAverage(int short_w, int long_w)
    : short_window(short_w), long_window(long_w) {
    if (short_window <= 0 || long_window <= 0) {
        throw std::invalid_argument("Window sizes must be positive.");
    }
    if (short_window >= long_window) {
        throw std::invalid_argument("Short window must be smaller than long window.");
    }
}

std::vector<int> SimpleMovingAverage::generate_signals(const std::vector<DataPoint>& data) {
    std::vector<int> signals(data.size(), 0); // 0: Hold, 1: Buy, -1: Sell

    if (data.size() < static_cast<size_t>(long_window)) {
        // Not enough data to generate signals
        return signals;
    }

    std::vector<double> short_mavg(data.size());
    std::vector<double> long_mavg(data.size());

    // Calculate short moving average
    for (size_t i = 0; i < data.size(); ++i) {
        if (i + 1 >= static_cast<size_t>(short_window)) {
            double sum = 0;
            for (int j = 0; j < short_window; ++j) {
                sum += data[i - j].close; // Use .close instead of .price
            }
            short_mavg[i] = sum / short_window;
        } else {
            short_mavg[i] = 0; // Or some other placeholder for insufficient data
        }
    }

    // Calculate long moving average
    for (size_t i = 0; i < data.size(); ++i) {
        if (i + 1 >= static_cast<size_t>(long_window)) {
            double sum = 0;
            for (int j = 0; j < long_window; ++j) {
                sum += data[i - j].close; // Use .close instead of .price
            }
            long_mavg[i] = sum / long_window;
        } else {
            long_mavg[i] = 0; // Or some other placeholder for insufficient data
        }
    }

    // Generate signals based on crossover
    for (size_t i = 1; i < data.size(); ++i) {
        if (static_cast<size_t>(long_window) > i) continue; // Ensure we have enough data for long_mavg

        // Buy signal: short_mavg crosses above long_mavg
        if (short_mavg[i-1] <= long_mavg[i-1] && short_mavg[i] > long_mavg[i]) {
            signals[i] = 1;
        }
        // Sell signal: short_mavg crosses below long_mavg
        else if (short_mavg[i-1] >= long_mavg[i-1] && short_mavg[i] < long_mavg[i]) {
            signals[i] = -1;
        }
    }

    return signals;
}
