#pragma once

#include "strategy_base.h"
#include "utils/config.h"
#include <string>
#include <vector>
#include <memory>

namespace backtest {

class SimpleSMABroadStrategy : public StrategyBase {
public:
    SimpleSMABroadStrategy(const StrategyId& id,
                           int short_ema, int long_ema, int rsi_period, double rsi_lb, double rsi_ub,
                           int atr_period, int adx_period, double adx_threshold,
                           double risk_per_trade, double initial_capital, double slippage,
                           double max_daily_drawdown);
    void initialize() override;
    void on_market_data(const MarketEvent& event) override;
    void on_fill(const FillEvent& event) override;
private:
    // Configurable parameters
    int short_ema, long_ema, rsi_period, atr_period, adx_period;
    double rsi_lb, rsi_ub, adx_threshold, risk_per_trade, initial_capital, slippage, max_daily_drawdown;
    // State
    double equity, daily_peak;
    int position;
    double entry_price, stop_level, tp_level, original_stop_distance;
    std::string log_path;
    std::vector<std::string> trade_logs;
    std::string last_date;
    // Indicator state
    std::vector<double> close, high, low, volume;
    std::vector<std::string> datetime;
    // Member variables for indicator history
    std::vector<double> tr_hist_m;
    std::vector<double> plus_dm_hist_m;
    std::vector<double> minus_dm_hist_m;
    std::vector<double> dx_hist_m;
    int print_count_m; // For debugging tick printing

    void log_trade(const std::string& log_line);
    void flush_logs();
};

} // namespace backtest
