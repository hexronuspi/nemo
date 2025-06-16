#include "core/engine.h"
#include "algo/simple_moving_average.h"
#include "metrics/backtester.h"
#include "data_loader.h"
#include "utils/logging.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <iomanip>

using namespace backtest;

int main(int argc, char* argv[]) {
    try {
        // Initialize logging
        Logger::get().init("logs/simpleSMABroad_trades.log", true, LogLevel::INFO);
        Logger::get().start();
        Logger& logger = Logger::get();

        // Set parameters directly
        std::string data_file = "data/stock_data.csv";
        int short_window = 12;
        int long_window = 26;
        double initial_capital = 100000.0;

        // Load market data
        DataLoader loader;
        std::vector<DataPoint> data = loader.load_data(data_file);
        logger.info("main", std::string("Market data loaded from: ") + data_file);

        // Create and run strategy
        SimpleMovingAverage sma_algo(short_window, long_window);
        std::vector<int> signals = sma_algo.generate_signals(data);
        logger.info("main", "Signals generated using SimpleMovingAverage");

        // Run backtest
        Backtester backtester(initial_capital);
        backtester.run_simulation(data, signals);
        logger.info("main", "Backtest simulation completed");

        // Print results
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n==== BACKTEST RESULTS SUMMARY ====" << std::endl;
        std::cout << "Total P&L: $" << backtester.get_pnl() << std::endl;
        std::cout << "Total Trades: " << backtester.get_num_trades() << std::endl;
        std::cout << "Average Trade PnL: $" << backtester.get_average_trade_pnl() << std::endl;
        std::cout << "Win Rate: " << (backtester.get_win_rate() * 100) << "%" << std::endl;
        std::cout << "Max Drawdown: " << (backtester.get_max_drawdown() * 100) << "%" << std::endl;
        std::cout << "==================================\n" << std::endl;

        Logger::get().stop();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        Logger::get().stop();
        return 1;
    }
}
