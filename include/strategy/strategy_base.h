#pragma once

#include "core/events.h"
#include "utils/types.h"
#include "utils/logging.h"
#include <memory>
#include <unordered_map>

namespace backtest {

// Base class for all trading strategies
class StrategyBase {
public:
    explicit StrategyBase(const StrategyId& strategy_id) 
        : strategy_id_(strategy_id) {}
    
    virtual ~StrategyBase() = default;
    
    // Strategy lifecycle
    virtual void initialize() {}
    virtual void on_start() {}
    virtual void on_stop() {}
    virtual void on_pause() {}
    virtual void on_resume() {}
    
    // Event handlers
    virtual void on_market_data(const MarketEvent& event) = 0;
    virtual void on_fill(const FillEvent& event) {}
    virtual void on_risk_event(const RiskEvent& event) {}
    virtual void on_timer(const TimerEvent& event) {}
    
    // Strategy identification
    const StrategyId& id() const { return strategy_id_; }
    
    // Position tracking
    const std::unordered_map<InstrumentId, Position>& positions() const { return positions_; }
    const Position* get_position(const InstrumentId& instrument) const {
        auto it = positions_.find(instrument);
        return (it != positions_.end()) ? &it->second : nullptr;
    }
    
    // Performance tracking
    Price get_total_pnl() const { return total_pnl_; }
    Price get_realized_pnl() const { return realized_pnl_; }
    Price get_unrealized_pnl() const { return unrealized_pnl_; }
    size_t get_trade_count() const { return trade_count_; }
    
    // Strategy state
    bool is_active() const { return is_active_; }
    void set_active(bool active) { is_active_ = active; }
    
protected:
    // Signal generation helpers
    void emit_signal(const InstrumentId& instrument, SignalEvent::SignalType signal_type, 
                    Price strength = 1.0) const;
    void execute_order(const InstrumentId& instrument, Side side, Price price, Volume qty = 1) const;
    
    void emit_buy_signal(const InstrumentId& instrument, Price strength = 1.0) const {
        emit_signal(instrument, SignalEvent::SignalType::BUY, strength);
    }
    void emit_sell_signal(const InstrumentId& instrument, Price strength = 1.0) const {
        emit_signal(instrument, SignalEvent::SignalType::SELL, strength);
    }
    void emit_close_signal(const InstrumentId& instrument) const {
        emit_signal(instrument, SignalEvent::SignalType::CLOSE, 1.0);
    }
    
    StrategyId strategy_id_;
    std::unordered_map<InstrumentId, Position> positions_;
    Price total_pnl_ = 0.0;
    Price realized_pnl_ = 0.0;
    Price unrealized_pnl_ = 0.0;
    size_t trade_count_ = 0;
    bool is_active_ = true;
    // REMOVE: mutable Logger logger_;
    // Use Logger::get() for logging in all strategies
};

// Simple Moving Average Crossover Strategy
class SMAStrategy : public StrategyBase {
public:
    enum class PriceMode { CLOSE, OPEN, HIGH, LOW, HLC3, OHLC4 };
    SMAStrategy(const StrategyId& strategy_id, int short_period, int long_period, PriceMode price_mode, std::unordered_map<std::string, std::string> price_columns)
        : StrategyBase(strategy_id), short_period_(short_period), long_period_(long_period), price_mode_(price_mode), price_columns_(std::move(price_columns)) {}
    ~SMAStrategy() override = default;
    void initialize() override;
    void on_market_data(const MarketEvent& event) override;
    void on_fill(const FillEvent& event) override;
    static PriceMode price_mode_from_string(const std::string& s);
private:
    struct PriceHistory {
        std::vector<Price> prices;
        bool has_signal = false;
    };
    int short_period_;
    int long_period_;
    PriceMode price_mode_;
    std::unordered_map<std::string, std::string> price_columns_;
    std::unordered_map<InstrumentId, PriceHistory> price_histories_;
};

// Mean Reversion Strategy
class MeanReversionStrategy : public StrategyBase {
public:
    MeanReversionStrategy(const StrategyId& strategy_id, int lookback_period = 20, 
                         double threshold = 2.0)
        : StrategyBase(strategy_id), lookback_period_(lookback_period), threshold_(threshold) {}
    ~MeanReversionStrategy() override = default;
    void initialize() override;
    void on_market_data(const MarketEvent& event) override;
    void on_fill(const FillEvent& event) override;
    
private:
    struct StatisticalData {
        std::vector<Price> prices;
        Price mean = 0.0;
        Price std_dev = 0.0;
        Price z_score = 0.0;
        
        void update(Price price, int lookback_period);
        bool is_oversold(double threshold) const { return z_score < -threshold; }
        bool is_overbought(double threshold) const { return z_score > threshold; }
    };
    
    int lookback_period_;
    double threshold_;
    std::unordered_map<InstrumentId, StatisticalData> statistical_data_;
};

// Momentum Strategy
class MomentumStrategy : public StrategyBase {
public:
    MomentumStrategy(const StrategyId& strategy_id, int lookback_period = 10, 
                    double threshold = 0.02)
        : StrategyBase(strategy_id), lookback_period_(lookback_period), threshold_(threshold) {}
    ~MomentumStrategy() override = default;
    void initialize() override;
    void on_market_data(const MarketEvent& event) override;
    void on_fill(const FillEvent& event) override;
    
private:
    struct MomentumData {
        std::vector<Price> prices;
        Price momentum = 0.0;
        
        void update(Price price, int lookback_period);
        bool has_positive_momentum(double threshold) const { return momentum > threshold; }
        bool has_negative_momentum(double threshold) const { return momentum < -threshold; }
    };
    
    int lookback_period_;
    double threshold_;
    std::unordered_map<InstrumentId, MomentumData> momentum_data_;
};

// Strategy factory
namespace StrategyFactory {
    std::unique_ptr<StrategyBase> create_sma_strategy(const StrategyId& id, 
                                                     int short_period = 12, 
                                                     int long_period = 26);
    std::unique_ptr<StrategyBase> create_sma_strategy(const StrategyId& id, int short_period, int long_period, SMAStrategy::PriceMode price_mode, std::unordered_map<std::string, std::string> price_columns);
    
    std::unique_ptr<StrategyBase> create_mean_reversion_strategy(const StrategyId& id,
                                                               int lookback = 20,
                                                               double threshold = 2.0);
    
    std::unique_ptr<StrategyBase> create_momentum_strategy(const StrategyId& id,
                                                         int lookback = 10,
                                                         double threshold = 0.02);
}

} // namespace backtest
