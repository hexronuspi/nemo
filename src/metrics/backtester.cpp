#include "metrics/backtester.h"
#include "logging.h"
#include <iostream>
#include <algorithm>

Backtester::Backtester(double init_cash)
    : initial_cash(init_cash), final_cash(init_cash) {}

void Backtester::run_simulation(const std::vector<DataPoint>& data, const std::vector<int>& signals) {
    bool in_position = false;
    Trade current_trade;

    for (size_t i = 0; i < data.size(); ++i) {
        if (signals[i] == 1 && !in_position) {
            current_trade.buy_index = i;
            current_trade.buy_price = data[i].close;
            in_position = true;

            Logger::get().log(std::chrono::system_clock::now(), {
                {"Type", "BUY"},
                {"Index", std::to_string(i)},
                {"Price", std::to_string(data[i].close)}
            });
        }
        else if (signals[i] == -1 && in_position) {
            current_trade.sell_index = i;
            current_trade.sell_price = data[i].close;
            current_trade.pnl = current_trade.sell_price - current_trade.buy_price;
            trades.push_back(current_trade);
            final_cash += current_trade.pnl;

            Logger::get().log(std::chrono::system_clock::now(), {
                {"Type", "SELL"},
                {"BuyIndex", std::to_string(current_trade.buy_index)},
                {"SellIndex", std::to_string(current_trade.sell_index)},
                {"BuyPrice", std::to_string(current_trade.buy_price)},
                {"SellPrice", std::to_string(current_trade.sell_price)},
                {"PnL", std::to_string(current_trade.pnl)}
            });

            in_position = false;
        }
    }
}

double Backtester::get_pnl() const {
    return final_cash - initial_cash;
}

size_t Backtester::get_num_trades() const {
    return trades.size();
}

double Backtester::get_average_trade_pnl() const {
    if (trades.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& trade : trades) sum += trade.pnl;
    return sum / trades.size();
}

double Backtester::get_win_rate() const {
    if (trades.empty()) return 0.0;
    size_t wins = 0;
    for (const auto& trade : trades) if (trade.pnl > 0) ++wins;
    return static_cast<double>(wins) / trades.size();
}

double Backtester::get_max_drawdown() const {
    double peak = initial_cash;
    double max_dd = 0.0;
    double current = initial_cash;

    for (const auto& trade : trades) {
        current += trade.pnl;
        if (current > peak) peak = current;
        double drawdown = (peak - current) / peak;
        if (drawdown > max_dd) max_dd = drawdown;
    }
    return max_dd;
}
