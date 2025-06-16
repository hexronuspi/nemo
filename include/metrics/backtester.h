#ifndef BACKTESTER_H
#define BACKTESTER_H

#include <vector>
#include "data_loader.h"

struct Trade {
    size_t buy_index;
    size_t sell_index;
    double buy_price;
    double sell_price;
    double pnl;
};

class Backtester {
private:
    std::vector<Trade> trades;
    std::vector<double> equity_curve;
    double initial_cash;
    double final_cash;

public:
    Backtester(double init_cash = 10000.0);
    void run_simulation(const std::vector<DataPoint>& data, const std::vector<int>& signals);
    double get_pnl() const;
    size_t get_num_trades() const;
    double get_average_trade_pnl() const;
    double get_win_rate() const;
    double get_max_drawdown() const;
};

#endif
