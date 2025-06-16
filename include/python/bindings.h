#pragma once

#include "core/engine.h"
#include "strategy/strategy_base.h"
#include "utils/types.h"
#include <memory>
#include <string>

namespace backtest {
namespace python {

// Python strategy wrapper
class PythonStrategy : public StrategyBase {
public:
    explicit PythonStrategy(const StrategyId& strategy_id, const std::string& python_module);
    
    void initialize() override;
    void on_market_data(const MarketEvent& event) override;
    void on_fill(const FillEvent& event) override;
    void on_risk_event(const RiskEvent& event) override;
    void on_timer(const TimerEvent& event) override;
    
private:
    std::string python_module_;
    void* python_strategy_instance_ = nullptr;  // PyObject* in implementation
    
    void call_python_method(const std::string& method_name, const std::vector<std::string>& args = {});
};

// Python API functions to be exposed via Pybind11
namespace api {
    
    // Engine control
    void initialize_engine(const std::string& config_file = "");
    void add_strategy_from_python(const std::string& strategy_id, const std::string& module_name);
    void load_data_file(const std::string& filepath);
    void run_backtest();
    void run_backtest_range(const std::string& start_date, const std::string& end_date);
    
    // Data access
    std::vector<double> get_prices(const std::string& instrument);
    std::vector<std::string> get_timestamps(const std::string& instrument);
    size_t get_data_size(const std::string& instrument);
    
    // Position and P&L
    double get_position(const std::string& strategy_id, const std::string& instrument);
    double get_strategy_pnl(const std::string& strategy_id);
    double get_total_pnl();
    
    // Order submission (for Python strategies)
    void submit_buy_order(const std::string& strategy_id, const std::string& instrument, 
                         double quantity, double price = 0.0);  // price=0 for market order
    void submit_sell_order(const std::string& strategy_id, const std::string& instrument, 
                          double quantity, double price = 0.0);
    
    // Signal emission
    void emit_buy_signal(const std::string& strategy_id, const std::string& instrument, double strength = 1.0);
    void emit_sell_signal(const std::string& strategy_id, const std::string& instrument, double strength = 1.0);
    void emit_close_signal(const std::string& strategy_id, const std::string& instrument);
    
    // Results export
    void export_results(const std::string& output_dir);
    std::string get_results_json();
    
    // Configuration
    void set_config_value(const std::string& key, const std::string& value);
    std::string get_config_value(const std::string& key);
    
    // Logging
    void log_info(const std::string& strategy_id, const std::string& message);
    void log_debug(const std::string& strategy_id, const std::string& message);
    void log_error(const std::string& strategy_id, const std::string& message);
}

// Initialize Python bindings (called once at startup)
void initialize_python_bindings();

// Cleanup Python bindings
void cleanup_python_bindings();

} // namespace python
} // namespace backtest

// Macro for easy Pybind11 module definition
#define BACKTEST_PYTHON_MODULE(module_name) \
    PYBIND11_MODULE(module_name, m) { \
        backtest::python::initialize_python_bindings(); \
        \
        m.doc() = "High-performance backtesting framework"; \
        \
        /* Engine control */ \
        m.def("initialize_engine", &backtest::python::api::initialize_engine, \
              "Initialize the backtesting engine", \
              pybind11::arg("config_file") = ""); \
        \
        m.def("add_strategy", &backtest::python::api::add_strategy_from_python, \
              "Add a Python strategy to the engine", \
              pybind11::arg("strategy_id"), pybind11::arg("module_name")); \
        \
        m.def("load_data", &backtest::python::api::load_data_file, \
              "Load market data from file", \
              pybind11::arg("filepath")); \
        \
        m.def("run", &backtest::python::api::run_backtest, \
              "Run the complete backtest"); \
        \
        m.def("run_range", &backtest::python::api::run_backtest_range, \
              "Run backtest for specific date range", \
              pybind11::arg("start_date"), pybind11::arg("end_date")); \
        \
        /* Data access */ \
        m.def("get_prices", &backtest::python::api::get_prices, \
              "Get price series for instrument", \
              pybind11::arg("instrument")); \
        \
        m.def("get_timestamps", &backtest::python::api::get_timestamps, \
              "Get timestamp series for instrument", \
              pybind11::arg("instrument")); \
        \
        /* Position and P&L */ \
        m.def("get_position", &backtest::python::api::get_position, \
              "Get current position for strategy and instrument", \
              pybind11::arg("strategy_id"), pybind11::arg("instrument")); \
        \
        m.def("get_strategy_pnl", &backtest::python::api::get_strategy_pnl, \
              "Get P&L for strategy", \
              pybind11::arg("strategy_id")); \
        \
        m.def("get_total_pnl", &backtest::python::api::get_total_pnl, \
              "Get total portfolio P&L"); \
        \
        /* Order submission */ \
        m.def("buy", &backtest::python::api::submit_buy_order, \
              "Submit buy order", \
              pybind11::arg("strategy_id"), pybind11::arg("instrument"), \
              pybind11::arg("quantity"), pybind11::arg("price") = 0.0); \
        \
        m.def("sell", &backtest::python::api::submit_sell_order, \
              "Submit sell order", \
              pybind11::arg("strategy_id"), pybind11::arg("instrument"), \
              pybind11::arg("quantity"), pybind11::arg("price") = 0.0); \
        \
        /* Signal emission */ \
        m.def("signal_buy", &backtest::python::api::emit_buy_signal, \
              "Emit buy signal", \
              pybind11::arg("strategy_id"), pybind11::arg("instrument"), \
              pybind11::arg("strength") = 1.0); \
        \
        m.def("signal_sell", &backtest::python::api::emit_sell_signal, \
              "Emit sell signal", \
              pybind11::arg("strategy_id"), pybind11::arg("instrument"), \
              pybind11::arg("strength") = 1.0); \
        \
        m.def("signal_close", &backtest::python::api::emit_close_signal, \
              "Emit close position signal", \
              pybind11::arg("strategy_id"), pybind11::arg("instrument")); \
        \
        /* Results */ \
        m.def("export_results", &backtest::python::api::export_results, \
              "Export backtest results", \
              pybind11::arg("output_dir")); \
        \
        m.def("get_results_json", &backtest::python::api::get_results_json, \
              "Get results as JSON string"); \
        \
        /* Configuration */ \
        m.def("set_config", &backtest::python::api::set_config_value, \
              "Set configuration value", \
              pybind11::arg("key"), pybind11::arg("value")); \
        \
        m.def("get_config", &backtest::python::api::get_config_value, \
              "Get configuration value", \
              pybind11::arg("key")); \
        \
        /* Logging */ \
        m.def("log_info", &backtest::python::api::log_info, \
              "Log info message", \
              pybind11::arg("strategy_id"), pybind11::arg("message")); \
        \
        m.def("log_debug", &backtest::python::api::log_debug, \
              "Log debug message", \
              pybind11::arg("strategy_id"), pybind11::arg("message")); \
        \
        m.def("log_error", &backtest::python::api::log_error, \
              "Log error message", \
              pybind11::arg("strategy_id"), pybind11::arg("message")); \
    }
