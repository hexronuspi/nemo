#pragma once

#include "core/event_bus.h"
#include "core/sim_clock.h"
#include "data/tick_data_store.h"
#include "execution/order_book.h"
#include "execution/cost_model.h"
#include "strategy/risk_manager.h"
#include "utils/logging.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace backtest {

// Forward declarations
class StrategyBase;
class ExecutionHandler;
class OrderRouter;

class BacktestEngine {
public:
    explicit BacktestEngine();
    ~BacktestEngine();
    
    // Initialize engine components
    void initialize();
    
    // Load market data
    void load_data(const std::string& filepath);
    void add_tick_data(const InstrumentId& instrument, const std::vector<MarketDataTick>& ticks);
    
    // Register strategies
    void add_strategy(std::unique_ptr<StrategyBase> strategy);
    
    // Configure components
    void set_cost_model(std::unique_ptr<CostModel> cost_model);
    void set_risk_limits(const RiskLimits& limits);
    void configure_latency(Duration market_data_latency = std::chrono::microseconds(1),
                          Duration order_latency = std::chrono::microseconds(100));
    
    // Run backtest
    void run();
    void run_range(Timestamp start_time, Timestamp end_time);
    
    // Control execution
    void pause();
    void resume();
    void stop();
    bool is_running() const { return is_running_; }
    
    // Get results
    struct BacktestResults {
        Timestamp start_time;
        Timestamp end_time;
        Duration total_duration;
        
        Price total_pnl = 0.0;
        Price total_commission = 0.0;
        Price total_slippage = 0.0;
        
        size_t total_trades = 0;
        size_t winning_trades = 0;
        size_t losing_trades = 0;
        
        Price max_drawdown = 0.0;
        Price max_profit = 0.0;
        Price sharpe_ratio = 0.0;
        
        std::unordered_map<StrategyId, Price> strategy_pnl;
        std::vector<Fill> trade_history;
        
        // Performance metrics
        Price win_rate() const {
            return total_trades > 0 ? static_cast<Price>(winning_trades) / total_trades : 0.0;
        }
        
        Price average_trade() const {
            return total_trades > 0 ? total_pnl / total_trades : 0.0;
        }
        
        Price profit_factor() const {
            Price gross_profit = 0.0;
            Price gross_loss = 0.0;
            Price position = 0.0;
            Price last_entry_price = 0.0;
            Side last_side = Side::BUY;
            for (const auto& fill : trade_history) {
                Price pnl = 0.0;
                if (fill.side == Side::SELL) {
                    pnl = (fill.price - last_entry_price) * fill.quantity;
                    if (pnl > 0) gross_profit += pnl;
                    else gross_loss += -pnl;
                } else if (fill.side == Side::BUY) {
                    last_entry_price = fill.price;
                    last_side = fill.side;
                }
            }
            return gross_loss > 0 ? gross_profit / gross_loss : 0.0;
        }
    };
    
    const BacktestResults& get_results() const { return results_; }
    
    // Export results
    void export_results(const std::string& output_dir) const;
    void export_trades_csv(const std::string& filepath) const;
    void export_summary_json(const std::string& filepath) const;
    void generate_report_markdown(const std::string& filepath) const;
    
    // Real-time monitoring
    void set_progress_callback(std::function<void(double)> callback) {
        progress_callback_ = callback;
    }
    
    void set_update_callback(std::function<void(const BacktestResults&)> callback) {
        update_callback_ = callback;
    }
    
    // Statistics
    struct EngineStats {
        size_t events_processed = 0;
        size_t orders_submitted = 0;
        size_t orders_filled = 0;
        size_t orders_rejected = 0;
        Duration total_processing_time;
        double events_per_second = 0.0;
    };
    
    const EngineStats& get_stats() const { return stats_; }
    
private:
    // Core components
    std::unique_ptr<EventBus> event_bus_;
    std::shared_ptr<SimClock> sim_clock_;
    std::unique_ptr<TickDataStore> data_store_;
    std::unique_ptr<RiskManager> risk_manager_;
    std::unique_ptr<CostModel> cost_model_;
    std::unique_ptr<ExecutionHandler> execution_handler_;
    std::unique_ptr<OrderRouter> order_router_;
    
    // Order books per instrument
    std::unordered_map<InstrumentId, std::unique_ptr<OrderBook>> order_books_;
    
    // Strategies
    std::vector<std::unique_ptr<StrategyBase>> strategies_;
    
    // State
    std::atomic<bool> is_running_{false};
    std::atomic<bool> is_paused_{false};
    std::atomic<bool> should_stop_{false};
    
    // Results and statistics
    BacktestResults results_;
    EngineStats stats_;
    
    // Callbacks
    std::function<void(double)> progress_callback_;
    std::function<void(const BacktestResults&)> update_callback_;
    
    // Latency settings
    Duration market_data_latency_{std::chrono::microseconds(1)};
    Duration order_latency_{std::chrono::microseconds(100)};
    
    // Event handlers
    SubscriptionHandle market_event_sub_;
    SubscriptionHandle signal_event_sub_;
    SubscriptionHandle order_event_sub_;
    SubscriptionHandle fill_event_sub_;
    SubscriptionHandle risk_event_sub_;
    
    // Processing methods
    void process_market_event(const MarketEvent& event);
    void process_signal_event(const SignalEvent& event);
    void process_order_event(const OrderEvent& event);
    void process_fill_event(const FillEvent& event);
    void process_risk_event(const RiskEvent& event);
    
    // Simulation control
    void advance_time_to(Timestamp target_time);
    void update_results();
    void update_progress();
    
    // Initialization helpers
    void setup_event_handlers();
    void create_order_books();
    void validate_configuration();
    
    // Statistics helpers
    void update_stats();
    void calculate_performance_metrics();
    
    static Logger logger_;
};

// Execution handler for order management
class ExecutionHandler {
public:
    ExecutionHandler(EventBus& event_bus, RiskManager& risk_manager,
                    CostModel& cost_model, Duration order_latency);
    
    void process_signal(const SignalEvent& event);
    void process_order(const OrderEvent& event);
    
    void set_order_books(std::unordered_map<InstrumentId, std::unique_ptr<OrderBook>>& order_books) {
        order_books_ = &order_books;
    }
    
private:
    EventBus& event_bus_;
    RiskManager& risk_manager_;
    CostModel& cost_model_;
    Duration order_latency_;
    
    std::unordered_map<InstrumentId, std::unique_ptr<OrderBook>>* order_books_ = nullptr;
    std::unordered_map<OrderId, Order> pending_orders_;
    OrderId next_order_id_ = 1;
    
    static Logger logger_;
};

// Order router for latency simulation
class OrderRouter {
public:
    OrderRouter(EventBus& event_bus, SimClock& sim_clock, Duration base_latency);
    
    void route_order(const Order& order);
    
private:
    EventBus& event_bus_;
    SimClock& sim_clock_;
    Duration base_latency_;
    
    static Logger logger_;
};

} // namespace backtest
