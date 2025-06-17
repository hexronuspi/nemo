#ifndef BACKTESTER_H
#define BACKTESTER_H

#include <vector>
#include <string>
#include "data_loader.h"

struct Trade {
    size_t buy_index;
    size_t sell_index;
    double buy_price;
    double sell_price;
    double pnl;
    size_t quantity; // number of units traded
    double capital_before;
    double capital_after;
    std::string buy_time;
    std::string sell_time;
    double commission; // total commission for the trade
};

class Backtester {
private:
    std::vector<Trade> trades;
    std::vector<double> equity_curve;
    double initial_cash;
    double final_cash;

public:
    Backtester(double init_cash = 10000.0);
    void run_simulation(const std::vector<DataPoint>& data, const std::vector<int>& signals, const std::string& column);
    double get_pnl() const;
    size_t get_num_trades() const;
    double get_average_trade_pnl() const;
    double get_win_rate() const;
    double get_max_drawdown() const;
    void export_trade_log(const std::vector<DataPoint>& data, const std::string& base_filename = "logs/simpleSMABroad_trades");
};

#endif
