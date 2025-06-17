#include "core/engine.h"
#include "algo/simple_moving_average.h"
#include "metrics/backtester.h"
#include "data_loader.h"
#include "utils/logging.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <iomanip>
#include <map>
#include <vector>

using namespace backtest;

int main(int argc, char* argv[]) {
    try {
        // Initialize logging
        Logger::get().init("logs/simpleSMABroad_trades.log", true, LogLevel::INFO);
        Logger::get().start();
        Logger& logger = Logger::get();

        // Example: Load multiple CSVs dynamically
        std::map<std::string, std::vector<DataPoint>> datasets;
        DataLoader loader;
        std::vector<std::string> data_names = {"data1"};
        std::vector<std::string> data_files = {"data/stock_data.csv"}; // Add more as needed
        for (size_t i = 0; i < data_names.size(); ++i) {
            datasets[data_names[i]] = loader.load_data(data_files[i]);
            logger.info("main", "Market data loaded from: " + data_files[i]);
        }

        // Example: Use data1 in algo, column name can be dynamic
        std::string column = "close"; // Or any column present in the CSV
        SimpleMovingAverage sma_algo(12, 26);
        std::vector<int> signals = sma_algo.generate_signals(datasets["data1"], column);
        logger.info("main", "Signals generated using SimpleMovingAverage");

        // Run backtest
        Backtester backtester(10000.0);
        backtester.run_simulation(datasets["data1"], signals, column);
        logger.info("main", "Backtest simulation completed");

        // Print results
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\n==== BACKTEST RESULTS SUMMARY ====" << std::endl;
        std::cout << "Initial Equity: $" << 10000.0 << std::endl;
        std::cout << "Final Equity: $" << backtester.get_pnl() + 10000.0 << std::endl;
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
