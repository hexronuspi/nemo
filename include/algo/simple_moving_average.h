#ifndef SIMPLE_MOVING_AVERAGE_H
#define SIMPLE_MOVING_AVERAGE_H

#include "algo/trading_algo.h"

class SimpleMovingAverage : public TradingAlgo {
private:
    int short_window;
    int long_window;
public:
    SimpleMovingAverage(int short_w, int long_w);
    std::vector<int> generate_signals(const std::vector<DataPoint>& data) override;
};

#endif 