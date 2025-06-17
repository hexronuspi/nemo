#include "metrics/backtester.h"
#include "utils/logging.h"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>

using backtest::Logger;

Backtester::Backtester(double init_cash)
    : initial_cash(init_cash), final_cash(init_cash) {}

void Backtester::run_simulation(const std::vector<DataPoint>& data, const std::vector<int>& signals, const std::string& column) {
    bool in_position = false;
    Trade current_trade;
    double equity = initial_cash;
    double net_pnl = 0.0;
    double max_equity = initial_cash;
    double min_equity = initial_cash;
    double sum_returns = 0.0;
    double sum_squared_returns = 0.0;
    size_t trade_count = 0;
    size_t win_count = 0;
    double last_trade_equity = initial_cash;
    size_t position = 0; // number of units held
    double buy_price = 0.0;
    std::string buy_time;
    equity_curve.clear();
    trades.clear();

    for (size_t i = 0; i < data.size(); ++i) {
        double price = 0.0;
        auto it = data[i].values.find(column);
        if (it != data[i].values.end()) price = it->second;
        // Buy signal
        if (signals[i] == 1 && !in_position) {
            size_t qty = static_cast<size_t>(equity / price);
            if (qty == 0) continue;
            buy_price = price;
            buy_time = data[i].values.count("timestamp") ? std::to_string(data[i].values.at("timestamp")) : std::to_string(i);
            double commission = 20.0; // $20 fee on buy
            current_trade = Trade{ i, 0, price, 0.0, 0.0, qty, equity, 0.0, buy_time, "", commission };
            equity -= qty * price;
            equity -= commission; // Deduct buy commission
            position = qty;
            in_position = true;
            Logger::get().log(std::chrono::system_clock::now(), {
                {"Type", "BUY"},
                {"Index", std::to_string(i)},
                {"Price", std::to_string(price)},
                {"Qty", std::to_string(qty)},
                {"CapitalBefore", std::to_string(current_trade.capital_before)},
                {"CapitalAfter", std::to_string(equity)},
                {"Commission", std::to_string(commission)}
            });
        }
        // Sell signal
        else if (signals[i] == -1 && in_position) {
            double sell_price = price;
            std::string sell_time = data[i].values.count("timestamp") ? std::to_string(data[i].values.at("timestamp")) : std::to_string(i);
            double trade_pnl = (sell_price - buy_price) * position;
            double commission = 20.0; // $20 fee on sell
            double profit_commission = 0.0;
            if (trade_pnl > 0) profit_commission = 0.05 * trade_pnl; // 5% on profit only
            commission += profit_commission;
            equity += position * sell_price;
            equity -= commission; // Deduct sell commission and profit commission
            current_trade.sell_index = i;
            current_trade.sell_price = sell_price;
            current_trade.pnl = trade_pnl - commission; // Net PnL after all fees
            current_trade.capital_after = equity;
            current_trade.sell_time = sell_time;
            current_trade.commission += commission; // total commission for round-trip
            trades.push_back(current_trade);
            net_pnl += current_trade.pnl;
            if (trade_pnl > 0) ++win_count;
            ++trade_count;
            double ret = (equity - last_trade_equity) / last_trade_equity;
            sum_returns += ret;
            sum_squared_returns += ret * ret;
            last_trade_equity = equity;
            if (equity > max_equity) max_equity = equity;
            if (equity < min_equity) min_equity = equity;
            equity_curve.push_back(equity);
            Logger::get().log(std::chrono::system_clock::now(), {
                {"Type", "SELL"},
                {"BuyIndex", std::to_string(current_trade.buy_index)},
                {"BuyPrice", std::to_string(current_trade.buy_price)},
                {"SellIndex", std::to_string(current_trade.sell_index)},
                {"SellPrice", std::to_string(current_trade.sell_price)},
                {"Qty", std::to_string(current_trade.quantity)},
                {"TradePnL", std::to_string(current_trade.pnl)},
                {"CapitalBefore", std::to_string(current_trade.capital_before)},
                {"CapitalAfter", std::to_string(current_trade.capital_after)},
                {"Equity", std::to_string(equity)},
                {"Commission", std::to_string(current_trade.commission)}
            });
            in_position = false;
            position = 0;
        }
    }
    final_cash = equity;
    // SOTA summary log
    double avg_trade = trade_count ? net_pnl / trade_count : 0.0;
    double win_rate = trade_count ? static_cast<double>(win_count) / trade_count : 0.0;
    double max_drawdown = 0.0;
    double peak = initial_cash;
    for (double eq : equity_curve) {
        if (eq > peak) peak = eq;
        double dd = (peak - eq) / peak;
        if (dd > max_drawdown) max_drawdown = dd;
    }
    double mean_return = trade_count ? sum_returns / trade_count : 0.0;
    double std_return = trade_count ? std::sqrt((sum_squared_returns / trade_count) - (mean_return * mean_return)) : 0.0;
    double sharpe = std_return > 0 ? mean_return / std_return * std::sqrt(252) : 0.0;
    Logger::get().log(std::chrono::system_clock::now(), {
        {"Type", "SUMMARY"},
        {"InitialEquity", std::to_string(initial_cash)},
        {"FinalEquity", std::to_string(final_cash)},
        {"TotalPnL", std::to_string(net_pnl)},
        {"NumTrades", std::to_string(trade_count)},
        {"AvgTradePnL", std::to_string(avg_trade)},
        {"WinRate", std::to_string(win_rate)},
        {"MaxDrawdown", std::to_string(max_drawdown)},
        {"Sharpe", std::to_string(sharpe)}
    });
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