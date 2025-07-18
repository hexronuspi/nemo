# Nemo: C++ Backtesting Engine


## 🏗️ Architecture Overview

Nemo's architecture is centered around an **Event Bus** that facilitates communication between its core components:

1.  **`BacktestEngine`**: Orchestrates the entire simulation, managing the event loop, strategies, and data flow.
2.  **`SimClock`**: Manages simulation time, crucial for event sequencing and latency simulation.
3.  **`TickDataStore`**: Efficiently stores and serves historical market data in a columnar format.
4.  **`DataLoader`**: Loads data from sources (e.g., CSV) into the `TickDataStore`.
5.  **`StrategyBase` (and derived C++/Python strategies)**: Implement trading logic, generating signals based on market data.
6.  **`ExecutionHandler`**: Processes signals, manages order lifecycle, and interacts with the `OrderBook`.
7.  **`OrderRouter`**: Simulates order routing and associated latencies.
8.  **`OrderBook`**: Simulates a limit order book for matching trades.
9.  **`RiskManager`**: Enforces risk limits (position, loss, exposure, etc.).
10. **`CostModel`**: Calculates transaction costs (slippage and commission).
11. **`Logger`**: Provides system-wide logging.

For a detailed component breakdown and data flow, please see [ARCHITECTURE.md](ARCHITECTURE.md).

## 📁 Project Structure

```
nemo/
├── CMakeLists.txt          # Main CMake build script
├── README.md               # This file
├── ARCHITECTURE.md         # Detailed architecture document
├── build.ps1               # PowerShell build script (Windows)
├── run_sample.ps1          # PowerShell script to run a sample
├── data/                   # Sample market data (e.g., stock_data.csv)
├── include/                # C++ header files
│   ├── algo/               # Algorithm-related headers (e.g., simple_moving_average.h)
│   ├── core/               # Core engine components (engine.h, event_bus.h, events.h, sim_clock.h)
│   ├── data/               # Data handling (tick_data_store.h, data_loader.h)
│   ├── execution/          # Order execution (order_book.h, cost_model.h, etc.)
│   ├── metrics/            # Performance metrics (backtester.h - can be integrated into engine results)
│   ├── python/             # Python bindings (bindings.h)
│   ├── strategy/           # Strategy components (strategy_base.h, risk_manager.h, simple_sma_broad.h)
│   └── utils/              # Utilities (logging.h, types.h, config.h)
├── src/                    # C++ source files (mirroring include/ structure)
│   ├── algo/
│   ├── core/
│   ├── data/
│   ├── execution/
│   ├── logger/
│   ├── metrics/
│   ├── python/
│   └── strategy/
│   └── main.cpp            # Main application entry point for C++ execution
├── strategies/             # Strategy implementations
│   └── python/             # Python strategy examples (e.g., sma_strategy.py)
├── logs/                   # Output directory for log files (e.g., simpleSMABroad_trades_YYYYMMDD_HHMMSS.log)
└── build/                  # Build output directory (created by CMake)
    └── bin/
        └── nemo.exe        # Compiled executable
```

## 🚀 Quick Start

### Prerequisites

*   C++17 compatible compiler (e.g., GCC, Clang, MSVC)
*   CMake (version 3.16 or higher)
*   Python (version 3.7+ for Python strategies and bindings)
*   (Optional) Pybind11 (often included as a submodule or found by CMake)

### Building the Engine

#### Using PowerShell (Windows - Recommended)

The `build.ps1` script simplifies the build process:

```powershell
# Clean previous build (optional)
./build.ps1 -Clean

# Build in Release mode (default)
./build.ps1

# Build in Debug mode
./build.ps1 -BuildType Debug
```
The executable `nemo.exe` will be in `build/bin/`.

#### Manual CMake Build (All Platforms)

1.  **Configure**:
    ```bash
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    ```
    (Replace `Release` with `Debug` for a debug build)

2.  **Build**:
    ```bash
    cmake --build build --config Release -j $(nproc) # For Linux/macOS with multiple cores
    # or
    cmake --build build --config Release # For Windows or single core
    ```
    The executable (`nemo` or `nemo.exe`) will be in `build/bin/`.

### Running a Backtest (C++ `main.cpp`)

The `src/main.cpp` provides an example of how to set up and run a backtest using a C++ strategy.

```powershell
# After building with PowerShell script
./build/bin/nemo.exe

# Or using the sample runner script
./run_sample.ps1
```

This will typically:
1.  Initialize the logger.
2.  Load data from `data/stock_data.csv`.
3.  Instantiate a strategy (e.g., `SimpleMovingAverage` or `SimpleSMABroadStrategy`).
4.  Run the backtest simulation.
5.  Print performance summary to the console.
6.  Generate log files in the `logs/` directory.

### Running with Python Strategies

(Assuming Python bindings are compiled)

1.  Ensure the `nemo` Python package (built by Pybind11) is in your `PYTHONPATH` or installed.
2.  Create a Python script:

    ```python
    import backtest_engine as bt
    import os

    def main():
        print("Initializing Nemo engine from Python...")
        # Engine setup (paths might need adjustment)
        # bt.initialize_engine() # If you have a global config
        
        # Configure Logger (example, C++ side might do this)
        # log_dir = "logs"
        # if not os.path.exists(log_dir):
        #     os.makedirs(log_dir)
        # log_file = os.path.join(log_dir, "python_sma_strategy_trades.log")
        # bt.set_config("logger.filepath", log_file) # Example config

        print("Loading data...")
        # Adjust path as necessary, assuming script is run from project root
        data_file = os.path.join("data", "stock_data.csv") 
        bt.load_data_file(data_file)

        print("Adding Python strategy...")
        # The module name is 'sma_strategy' if your file is 'strategies/python/sma_strategy.py'
        # The class within that module is 'SMAStrategy'
        # The factory function is 'create_strategy'
        bt.add_strategy_from_python(
            strategy_id="py_sma_1", 
            module_name="strategies.python.sma_strategy" # Path to module
        )
        # If add_strategy_from_python expects a factory:
        # bt.add_strategy_from_python("py_sma_1", "strategies.python.sma_strategy.create_strategy")

        print("Running backtest...")
        bt.run_backtest()

        print("Backtest finished. Results:")
        pnl = bt.get_total_pnl()
        print(f"Total P&L: ${pnl:.2f}")

        # Export results
        # results_dir = "results_output"
        # if not os.path.exists(results_dir):
        #     os.makedirs(results_dir)
        # bt.export_results(results_dir)
        # print(f"Results exported to {results_dir}")
        
        # print(f"Full results JSON: {bt.get_results_json()}")


    if __name__ == "__main__":
        main()
    ```

3.  Run the Python script:
    ```bash
    python your_python_script_name.py
    ```

### Log Files

*   **Application Logs**: General engine and strategy logs are written to files like `logs/simpleSMABroad_trades_YYYYMMDD_HHMMSS.log` (filename configured in `main.cpp` or via Python).
*   **Trade Logs**: Detailed trade-by-trade logs can be exported by the `BacktestEngine` (e.g., to CSV or JSON format). The `Backtester` class in `metrics/` also has functionality for this, which is being integrated more directly into the `BacktestEngine`'s results.

## 🛠️ Developing Strategies

### C++ Strategies

1.  Create a new class inheriting from `StrategyBase` (in `include/strategy/` and `src/strategy/`).
2.  Implement `initialize()`, `on_market_data()`, `on_fill()`, etc.
3.  Use `emit_buy_signal()`, `emit_sell_signal()`, `emit_close_signal()` to generate `SignalEvent`s.
4.  In `src/main.cpp` (or your C++ test runner), instantiate your strategy and add it to the `BacktestEngine`.

### Python Strategies

1.  Create a Python file in `strategies/python/` (e.g., `my_python_strategy.py`).
2.  Define a class with methods like `__init__`, `initialize`, `on_market_data`, `on_fill`.
    *   The `__init__` method should accept `strategy_id` and any other parameters.
    *   Use the `backtest_engine` module (imported as `bt`) to interact with the engine:
        *   `bt.signal_buy(self.strategy_id, instrument, ...)`
        *   `bt.log_info(self.strategy_id, "My log message")`
        *   Access P&L: `bt.get_strategy_pnl(self.strategy_id)`
3.  (Optional) Provide a factory function like `create_strategy(strategy_id: str, **kwargs)` in your Python module that returns an instance of your strategy. This is useful if `add_strategy_from_python` expects a factory.
4.  In your main Python script, use `bt.add_strategy_from_python("unique_strategy_id", "strategies.python.my_python_strategy")` to load it.
For a deep dive into the system's components and their interactions, refer to [ARCHITECTURE.md](ARCHITECTURE.md).
