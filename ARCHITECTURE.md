# C++ Backtesting Engine - Architecture

## Overview
This document outlines the architecture of the C++ Backtesting Engine, a framework designed for developing and testing quantitative trading strategies. It emphasizes a straightforward, code-driven approach with a focus on performance and clarity. The system processes historical market data, applies a trading strategy to generate signals, and then simulates trades to evaluate the strategy's performance.

## System Diagram

```
┌─────────────────┐      ┌───────────────────────┐      ┌───────────────────┐      ┌──────────────────┐
│  Market Data    │─────▶│    Data Loader        │─────▶│ Trading Algorithm │─────▶│    Backtester    │
│ (CSV File e.g., │      │ (Parses CSV into      │      │ (e.g., Simple     │      │ (Simulates Trades│
│ stock_data.csv) │      │  DataPoint structs)   │      │ Moving Average)   │      │ & Calculates P&L)│
└─────────────────┘      └───────────────────────┘      └───────────────────┘      └──────────────────┘
         │                         │                              │                          │
         └─────────────────────────┴───────────┬──────────────────┴──────────────────────────┘
                                               │
                                               ▼
                                     ┌──────────────────┐
                                     │      Logger      │
                                     │ (Records App &   │
                                     │   Trade Events)  │
                                     └──────────────────┘
```

## Core Components

### 1. Data Loading (`DataLoader`)
-   **Header**: `include/data_loader.h`
-   **Source**: `src/data_loader.cpp` (Assumed implementation)
-   **Responsibility**: Loads historical market data from CSV files.
-   **Input**: Path to a CSV file (e.g., `data/stock_data.csv`). The CSV is expected to have columns like timestamp, open, high, low, close, volume.
-   **Output**: A `std::vector<DataPoint>`, where `DataPoint` is a struct holding individual data records (OHLCV).
-   **`DataPoint` Struct**: Defined in `include/data_loader.h`, contains fields for timestamp, open, high, low, close, volume, and oi.

### 2. Trading Strategy (`TradingAlgo` & `SimpleMovingAverage`)
-   **Interface**: `include/algo/trading_algo.h` defines the `TradingAlgo` abstract base class.
    -   Key method: `virtual std::vector<int> generate_signals(const std::vector<DataPoint>& data) = 0;`
-   **Implementation Example**: `include/algo/simple_moving_average.h` and `src/algo/simple_moving_average.cpp` provide the `SimpleMovingAverage` strategy.
    -   **Logic**: Generates buy (1), sell (-1), or hold (0) signals based on the crossover of short-term and long-term simple moving averages of closing prices.
    -   **Parameters**: Short window and long window periods, passed during construction.

### 3. Backtesting & Metrics (`Backtester`)
-   **Header**: `include/metrics/backtester.h`
-   **Source**: `src/metrics/backtester.cpp`
-   **Responsibility**: Simulates trades based on the generated signals and historical data, then calculates performance metrics.
-   **Input**:
    -   A `std::vector<DataPoint>` (market data).
    -   A `std::vector<int>` (signals from the trading algorithm).
    -   Initial capital.
-   **Core Logic**:
    -   Iterates through data and signals.
    -   Executes trades (buy on signal 1, sell on signal -1 if in position).
    -   Tracks P&L, number of trades, win/loss statistics.
-   **Output**:
    -   Performance metrics: Total P&L, number of trades, average trade P&L, win rate, maximum drawdown.
    -   Exports a detailed trade log to TSV and CSV files (e.g., `logs/simpleSMABroad_trades.tsv/.csv`). The log includes entry/exit timestamps, prices, and P&L per trade.
-   **`Trade` Struct**: Defined in `include/metrics/backtester.h`, stores details of individual trades (buy/sell indices, prices, P&L).

### 4. Logging (`Logger`)
-   **Header**: `include/utils/logging.h` (Note: `include/logging.h` is empty/unused)
-   **Source**: `src/logger/logger.cpp`
-   **Responsibility**: Provides a flexible logging facility for application events and errors.
-   **Features**:
    -   Singleton access (`Logger::get()`).
    -   Initialization with log file path, option to remake (overwrite) or append, and minimum log level.
    -   Timestamped log entries.
    -   Structured logging with key-value fields.
    -   Used by `main.cpp` and `Backtester` to record significant events.
    -   The primary log file is `logs/simpleSMABroad_trades.log` as configured in `main.cpp`.

### 5. Main Application (`src/main.cpp`)
-   **Responsibility**: Orchestrates the entire backtesting process.
    1.  Initializes the `Logger`.
    2.  Sets parameters (data file path, strategy window sizes, initial capital).
    3.  Instantiates `DataLoader` and loads data.
    4.  Instantiates the chosen `TradingAlgo` (e.g., `SimpleMovingAverage`) and generates signals.
    5.  Instantiates `Backtester`, runs the simulation.
    6.  Prints summary performance metrics to the console.
    7.  Ensures the logger is stopped.

## Data Flow
1.  The `main` function in `src/main.cpp` starts the process.
2.  `DataLoader::load_data` is called to read market data from `data/stock_data.csv` into a `std::vector<DataPoint>`.
3.  An instance of a `TradingAlgo` (e.g., `SimpleMovingAverage`) is created. Its `generate_signals` method is called with the market data, returning a `std::vector<int>` of signals.
4.  An instance of `Backtester` is created with initial capital. Its `run_simulation` method is called with the market data and signals.
5.  The `Backtester` simulates trades, logs each buy/sell action using the `Logger`, and calculates performance metrics.
6.  After the simulation, `Backtester::export_trade_log` is called (implicitly or explicitly, currently called from `main.cpp` after simulation in a typical setup, though the provided `main.cpp` doesn't explicitly call it but `Backtester` logs trades during simulation). The current `Backtester` in `backtester.cpp` has an `export_trade_log` method that *can* be called.
7.  `main` retrieves and prints performance metrics from the `Backtester` to the standard output.
8.  Throughout the process, components use `Logger::get().log(...)` (or convenience methods like `info`, `error`) to write to `logs/simpleSMABroad_trades.log`.

## Build System (CMake)
-   **File**: `CMakeLists.txt`
-   **Configuration**:
    -   Sets C++17 standard.
    -   Specifies include directories (`include/`).
    -   Recursively finds all `.cpp` files in `src/` and its subdirectories.
    -   Builds a single executable named `nemo` (output to `build/bin/nemo`).
    -   Includes custom commands to copy `data/` and `logs/` directories to the build output directory after a successful build for convenience (though `logs/` is primarily runtime generated).

## Extending the Engine
-   **Adding New Trading Strategies**:
    -   Create a new class that inherits from `TradingAlgo` (in `include/algo/` and `src/algo/`).
    -   Implement the `generate_signals` method with your custom logic.
    -   In `src/main.cpp`, instantiate your new strategy instead of `SimpleMovingAverage`.
-   **Modifying Data Loading**:
    -   If `src/data_loader.cpp` exists, modify its parsing logic for different file formats or data sources.
    -   Adjust the `DataPoint` struct if new data fields are needed.
-   **Enhancing Backtester**:
    -   Add more sophisticated performance metrics to `Backtester`.
    -   Implement more complex cost models (e.g., slippage, commissions) within the `run_simulation` logic.

---

This architecture provides a solid foundation for C++ based backtesting. Its simplicity and directness make it easy to understand, modify, and extend.
