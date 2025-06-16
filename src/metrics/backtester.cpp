#include "metrics/backtester.h"
#include "utils/logging.h"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

using backtest::Logger;

Backtester::Backtester(double init_cash)
    : initial_cash(init_cash), final_cash(init_cash) {}

void Backtester::run_simulation(const std::vector<DataPoint>& data, const std::vector<int>& signals) {
    bool in_position = false;
    Trade current_trade;
    double equity = initial_cash;
    double net_pnl = 0.0;

    for (size_t i = 0; i < data.size(); ++i) {
        if (signals[i] == 1 && !in_position) {
            current_trade.buy_index = i;
            current_trade.buy_price = data[i].close;
            in_position = true;
            Logger::get().log(std::chrono::system_clock::now(), {
                {"Type", "BUY"},
                {"Index", std::to_string(i)},
                {"Timestamp", data[i].timestamp},
                {"Price", std::to_string(data[i].close)}
            });
        }
        else if (signals[i] == -1 && in_position) {
            current_trade.sell_index = i;
            current_trade.sell_price = data[i].close;
            current_trade.pnl = current_trade.sell_price - current_trade.buy_price;
            trades.push_back(current_trade);
            net_pnl += current_trade.pnl;
            equity += current_trade.pnl;
            final_cash = equity;
            // Log only the sell/exit event, one line, neat
            Logger::get().log(std::chrono::system_clock::now(), {
                {"Type", "SELL"},
                {"BuyIndex", std::to_string(current_trade.buy_index)},
                {"BuyTimestamp", data[current_trade.buy_index].timestamp},
                {"BuyPrice", std::to_string(current_trade.buy_price)},
                {"SellIndex", std::to_string(current_trade.sell_index)},
                {"SellTimestamp", data[current_trade.sell_index].timestamp},
                {"SellPrice", std::to_string(current_trade.sell_price)},
                {"TradePnL", std::to_string(current_trade.pnl)},
                {"NetPnL", std::to_string(net_pnl)},
                {"Equity", std::to_string(equity)}
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

void Backtester::export_trade_log(const std::vector<DataPoint>& data, const std::string& base_filename) {
    // Write TSV
    std::string tsv_file = base_filename + ".tsv";
    std::ofstream tsv(tsv_file);
    tsv << "Event\tEntryIndex\tEntryTime\tEntryPrice\tExitIndex\tExitTime\tExitPrice\tTradePnL\tCumulativePnL\tEquity\n";
    double equity = initial_cash;
    double net_pnl = 0.0;
    for (const auto& trade : trades) {
        net_pnl += trade.pnl;
        equity += trade.pnl;
        // Entry row
        tsv << "ENTRY\t" << trade.buy_index << "\t" << data[trade.buy_index].timestamp << "\t" << trade.buy_price << "\t\t\t\t\t\t\n";
        // Exit row
        tsv << "EXIT\t" << trade.buy_index << "\t" << data[trade.buy_index].timestamp << "\t" << trade.buy_price << "\t"
            << trade.sell_index << "\t" << data[trade.sell_index].timestamp << "\t" << trade.sell_price << "\t"
            << trade.pnl << "\t" << net_pnl << "\t" << equity << "\n";
    }
    tsv.close();
    // Write CSV
    std::string csv_file = base_filename + ".csv";
    std::ofstream csv(csv_file);
    csv << "Event,EntryIndex,EntryTime,EntryPrice,ExitIndex,ExitTime,ExitPrice,TradePnL,CumulativePnL,Equity\n";
    equity = initial_cash;
    net_pnl = 0.0;
    for (const auto& trade : trades) {
        net_pnl += trade.pnl;
        equity += trade.pnl;
        // Entry row
        csv << "ENTRY," << trade.buy_index << "," << data[trade.buy_index].timestamp << "," << trade.buy_price << ",,,,,,\n";
        // Exit row
        csv << "EXIT," << trade.buy_index << "," << data[trade.buy_index].timestamp << "," << trade.buy_price << ","
            << trade.sell_index << "," << data[trade.sell_index].timestamp << "," << trade.sell_price << ","
            << trade.pnl << "," << net_pnl << "," << equity << "\n";
    }
    csv.close();
}
