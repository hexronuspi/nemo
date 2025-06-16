#ifndef TRADING_ALGO_H
#define TRADING_ALGO_H

#include <vector>
#include "data_loader.h"

class TradingAlgo {
public:
    virtual std::vector<int> generate_signals(const std::vector<DataPoint>& data) = 0;
    virtual ~TradingAlgo() = default;
};

#endif