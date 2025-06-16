# C++ Backtesting Engine 🚀

A **streamlined, C++17 based backtesting framework** designed for quantitative analysis and trading strategy evaluation. Features clean logging and a simple, robust build process using CMake.

## 🏗️ **Core Architecture**

The engine follows a straightforward data processing pipeline:

1.  **Data Loading**: Historical market data is loaded from CSV files.
2.  **Signal Generation**: A trading algorithm processes the market data to produce buy/sell signals.
3.  **Backtesting**: The signals are fed into a backtester which simulates trades and calculates performance metrics.
4.  **Logging**: Comprehensive logs, including detailed trade logs, are generated.

## 📁 **Project Structure**

```
backtest/
├── CMakeLists.txt          # Main CMake build script
├── README.md               # This file
├── ARCHITECTURE.md         # Detailed architecture overview
├── data/                   # Sample market data (e.g., stock_data.csv)
├── include/                # C++ header files
│   ├── algo/               # Trading algorithm interfaces and implementations
│   │   ├── simple_moving_average.h
│   │   └── trading_algo.h
│   ├── metrics/            # Backtesting and performance metrics
│   │   └── backtester.h
│   ├── utils/              # Utility headers (e.g., logging)
│   │   └── logging.h
│   └── data_loader.h       # Market data loading interface
├── src/                    # C++ source files
│   ├── algo/               # Trading algorithm implementations
│   │   └── simple_moving_average.cpp
│   ├── logger/             # Logging implementation
│   │   └── logger.cpp
│   ├── metrics/            # Backtesting implementation
│   │   └── backtester.cpp
│   ├── main.cpp            # Main application entry point
│   └── data_loader.cpp     # Market data loading implementation (Assumed)
└── logs/                   # Output directory for log files
```
*(Note: `src/data_loader.cpp` is assumed to exist and implement the interface in `include/data_loader.h`)*

## ⚡ **Key Features**

- **Modern CMake Build**: Easily configurable and buildable with a single command (`cmake -S . -B build && cmake --build build`). The executable `nemo` will be created in `build/bin/`.
- **No Configuration File Dependency**: Parameters are managed directly within the code (see `src/main.cpp`), simplifying setup and execution.
- **Clean & Professional Logging**:
    - General application logs are written to `logs/simpleSMABroad_trades.log` (configurable in `src/main.cpp`).
    - Detailed trade logs are exported by the `Backtester` in both TSV and CSV formats (e.g., `logs/simpleSMABroad_trades.tsv`, `logs/simpleSMABroad_trades.csv`).
- **Example Strategy**: Includes a `SimpleMovingAverage` crossover strategy.
- **Core Performance Metrics**: The `Backtester` calculates and reports:
    - Total Profit & Loss (P&L)
    - Total Number of Trades
    - Average P&L per Trade
    - Win Rate
    - Maximum Drawdown

## 🚀 **Quick Start**

### 1. Prerequisites
- C++17 compatible compiler (e.g., GCC, Clang, MSVC)
- CMake (version 3.16 or higher)

### 2. Build the Engine

Open your terminal in the project's root directory and run:

```powershell
# Configure the project
cmake -S . -B build

# Build the project
cmake --build build
```
This will create the executable `nemo` (or `nemo.exe` on Windows) inside the `build/bin/` directory.

### 3. Run a Backtest

Execute the compiled program:

```powershell
# On Windows
build\\bin\\nemo.exe

# On Linux/macOS
./build/bin/nemo
```
The application will load data from `data/stock_data.csv`, run the `SimpleMovingAverage` strategy, and print backtest results to the console.

### 4. Review Logs
- General logs can be found in `logs/simpleSMABroad_trades.log`.
- Detailed trade-by-trade logs are in `logs/simpleSMABroad_trades.tsv` and `logs/simpleSMABroad_trades.csv`.

## 🛠️ **Extending the Engine**

### Adding a New Strategy
1.  Define your new strategy class by inheriting from `TradingAlgo` (see `include/algo/trading_algo.h`).
2.  Implement the `generate_signals` method.
3.  Instantiate and use your new strategy in `src/main.cpp`.

## 📝 **Contributing**
Contributions are welcome! Please fork the repository, create a new branch for your features or fixes, and submit a pull request. Ensure your code builds and adheres to the existing style.

---

For a more detailed component overview, see [ARCHITECTURE.md](ARCHITECTURE.md).