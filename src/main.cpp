#include "data_loader.h"
#include "algo/simple_moving_average.h"
#include "metrics/backtester.h"
#include "logging.h"

#include <iostream>
#include <iomanip>
#include <map>
#include <chrono>

int main() {
    try {
        Logger::get().init("../logs/log.txt", true);  // false = append; true = remake with timestamp

        Logger::get().log(std::chrono::system_clock::now(), {
            {"stage", "start"}, {"message", "Backtesting started"}
        });

        std::cout << "Starting backtesting run...\n";

        DataLoader loader;
        auto data = loader.load_data("../data/stock_data.csv");
        std::cout << "Loaded " << data.size() << " data points.\n";

        Logger::get().log(std::chrono::system_clock::now(), {
            {"stage", "data_loaded"}, {"rows", std::to_string(data.size())}
        });

        SimpleMovingAverage algo(12, 26);
        auto signals = algo.generate_signals(data);
        std::cout << "Generated trading signals.\n";

        Logger::get().log(std::chrono::system_clock::now(), {
            {"stage", "signals_generated"}, {"signals", std::to_string(signals.size())}
        });

        Backtester backtester;
        backtester.run_simulation(data, signals);
        std::cout << "Simulation completed.\n";

        Logger::get().log(std::chrono::system_clock::now(), {
            {"stage", "simulation_done"}, {"trades", std::to_string(backtester.get_num_trades())}
        });

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Results:\n";
        std::cout << "- Total P&L: $" << backtester.get_pnl() << "\n";
        std::cout << "- Number of Trades: " << backtester.get_num_trades() << "\n";
        std::cout << "- Average Trade P&L: $" << backtester.get_average_trade_pnl() << "\n";
        std::cout << "- Win Rate: " << (backtester.get_win_rate() * 100) << "%\n";
        std::cout << "- Maximum Drawdown: " << (backtester.get_max_drawdown() * 100) << "%\n";

        Logger::get().log(std::chrono::system_clock::now(), {
            {"stage", "results_logged"},
            {"TotalPnL", std::to_string(backtester.get_pnl())},
            {"NumTrades", std::to_string(backtester.get_num_trades())},
            {"AvgPnL", std::to_string(backtester.get_average_trade_pnl())},
            {"WinRate", std::to_string(backtester.get_win_rate())},
            {"MaxDrawdown", std::to_string(backtester.get_max_drawdown())}
        });

        Logger::get().log(std::chrono::system_clock::now(), {
            {"stage", "completed"}, {"message", "Backtesting finished"}
        });

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        Logger::get().log(std::chrono::system_clock::now(), {
            {"stage", "error"}, {"exception", e.what()}
        });
        return 1;
    }

    std::cout << "Backtesting run completed.\n";
    return 0;
}
