#pragma once

#include "utils/types.h"
#include "core/events.h"
#include <unordered_map>
#include <chrono>
#include <functional>
#include <queue>
#include <mutex>

namespace backtest {

// Hash function for pair keys (now top-level)
struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const {
        auto h1 = std::hash<T1>{}(pair.first);
        auto h2 = std::hash<T2>{}(pair.second);
        return h1 ^ (h2 << 1);
    }
};

// Risk limits configuration
struct RiskLimits {
    // Position limits
    Volume max_position_size = 1000000;         // Maximum position size per instrument
    Price max_notional_exposure = 10000000.0;   // Maximum notional exposure per instrument
    Price max_portfolio_exposure = 50000000.0;  // Maximum total portfolio exposure
    
    // Loss limits
    Price max_daily_loss = -10000.0;            // Maximum daily loss (negative)
    Price max_total_loss = -50000.0;            // Maximum total loss (negative)
    Price max_drawdown = -0.1;                  // Maximum drawdown as percentage
    
    // Trading limits
    uint32_t max_orders_per_minute = 100;       // Rate limiting
    uint32_t max_orders_per_day = 10000;        // Daily order limit
    Volume max_order_size = 10000;              // Maximum single order size
    
    // Cooldown periods
    std::chrono::minutes loss_cooldown{30};     // Cooldown after significant loss
    std::chrono::minutes drawdown_cooldown{60}; // Cooldown after drawdown threshold
    
    // Risk checks
    bool enable_position_limits = true;
    bool enable_loss_limits = true;
    bool enable_exposure_limits = true;
    bool enable_rate_limiting = true;
};

// Risk check result
enum class RiskCheckResult {
    APPROVED,
    REJECTED_POSITION_LIMIT,
    REJECTED_EXPOSURE_LIMIT,
    REJECTED_LOSS_LIMIT,
    REJECTED_ORDER_SIZE,
    REJECTED_RATE_LIMIT,
    REJECTED_COOLDOWN
};

// Risk violation information
struct RiskViolation {
    RiskCheckResult result;
    std::string message;
    Price current_value;
    Price limit_value;
};

class RiskManager {
public:
    explicit RiskManager(const RiskLimits& limits = RiskLimits{})
        : limits_(limits) {}
    
    // Set risk limits
    void set_limits(const RiskLimits& limits) {
        std::lock_guard<std::mutex> lock(mutex_);
        limits_ = limits;
    }
    
    // Set strategy-specific limits
    void set_strategy_limits(const StrategyId& strategy, const RiskLimits& limits) {
        std::lock_guard<std::mutex> lock(mutex_);
        strategy_limits_[strategy] = limits;
    }
    
    // Pre-trade risk check
    std::optional<RiskViolation> check_order(const Order& order) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        const auto& limits = get_limits(order.strategy);
        auto now = std::chrono::high_resolution_clock::now();
        
        // Check order size
        if (limits.enable_position_limits && order.quantity > limits.max_order_size) {
            return RiskViolation{
                RiskCheckResult::REJECTED_ORDER_SIZE,
                "Order size exceeds maximum allowed",
                static_cast<Price>(order.quantity),
                static_cast<Price>(limits.max_order_size)
            };
        }
        
        // Check rate limiting
        if (limits.enable_rate_limiting) {
            auto& rate_data = rate_limiting_[order.strategy];
            
            // Clean old orders (older than 1 minute)
            auto minute_ago = now - std::chrono::minutes(1);
            while (!rate_data.order_times.empty() && rate_data.order_times.front() < minute_ago) {
                rate_data.order_times.pop();
            }
            
            if (rate_data.order_times.size() >= limits.max_orders_per_minute) {
                return RiskViolation{
                    RiskCheckResult::REJECTED_RATE_LIMIT,
                    "Order rate limit exceeded",
                    static_cast<Price>(rate_data.order_times.size()),
                    static_cast<Price>(limits.max_orders_per_minute)
                };
            }
            
            // Check daily order limit
            if (rate_data.daily_orders >= limits.max_orders_per_day) {
                return RiskViolation{
                    RiskCheckResult::REJECTED_RATE_LIMIT,
                    "Daily order limit exceeded",
                    static_cast<Price>(rate_data.daily_orders),
                    static_cast<Price>(limits.max_orders_per_day)
                };
            }
        }
        
        // Check position limits
        if (limits.enable_position_limits) {
            auto& position = positions_[{order.strategy, order.instrument}];
            Volume new_position = position.quantity;
            
            if (order.side == Side::BUY) {
                new_position += order.quantity;
            } else {
                new_position -= order.quantity;
            }
            
            if (std::abs(static_cast<int64_t>(new_position)) > limits.max_position_size) {
                return RiskViolation{
                    RiskCheckResult::REJECTED_POSITION_LIMIT,
                    "Position size limit exceeded",
                    static_cast<Price>(std::abs(static_cast<int64_t>(new_position))),
                    static_cast<Price>(limits.max_position_size)
                };
            }
        }
        
        // Check exposure limits
        if (limits.enable_exposure_limits) {
            Price notional = order.quantity * order.price;
            auto& exposure = exposures_[{order.strategy, order.instrument}];
            
            if (notional > limits.max_notional_exposure) {
                return RiskViolation{
                    RiskCheckResult::REJECTED_EXPOSURE_LIMIT,
                    "Notional exposure limit exceeded",
                    notional,
                    limits.max_notional_exposure
                };
            }
        }
        
        // Check loss limits and cooldowns
        if (limits.enable_loss_limits) {
            auto& pnl_data = strategy_pnl_[order.strategy];
            
            if (pnl_data.daily_pnl < limits.max_daily_loss) {
                return RiskViolation{
                    RiskCheckResult::REJECTED_LOSS_LIMIT,
                    "Daily loss limit exceeded",
                    pnl_data.daily_pnl,
                    limits.max_daily_loss
                };
            }
            
            if (pnl_data.total_pnl < limits.max_total_loss) {
                return RiskViolation{
                    RiskCheckResult::REJECTED_LOSS_LIMIT,
                    "Total loss limit exceeded",
                    pnl_data.total_pnl,
                    limits.max_total_loss
                };
            }
            
            // Check cooldown periods
            if (pnl_data.cooldown_until > now) {
                auto remaining = std::chrono::duration_cast<std::chrono::minutes>(
                    pnl_data.cooldown_until - now);
                return RiskViolation{
                    RiskCheckResult::REJECTED_COOLDOWN,
                    "Strategy in cooldown period, remaining: " + std::to_string(remaining.count()) + " minutes",
                    0.0,
                    0.0
                };
            }
        }
        
        return std::nullopt;  // Order approved
    }
    
    // Record order submission
    void on_order_submitted(const Order& order) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (limits_.enable_rate_limiting) {
            auto& rate_data = rate_limiting_[order.strategy];
            rate_data.order_times.push(order.timestamp);
            rate_data.daily_orders++;
        }
    }
    
    // Update position after fill
    void on_fill(const Fill& fill) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto position_key = std::make_pair(fill.strategy, fill.instrument);
        auto& position = positions_[position_key];
        
        // Update position
        if (fill.side == Side::BUY) {
            position.quantity += fill.quantity;
        } else {
            position.quantity -= fill.quantity;
        }
        
        // Update exposure
        auto& exposure = exposures_[position_key];
        exposure += fill.quantity * fill.price;
        
        // Update P&L
        auto& pnl_data = strategy_pnl_[fill.strategy];
        Price trade_pnl = calculate_trade_pnl(fill, position);
        pnl_data.daily_pnl += trade_pnl;
        pnl_data.total_pnl += trade_pnl;
        
        // Check for cooldown triggers
        const auto& limits = get_limits(fill.strategy);
        if (limits.enable_loss_limits) {
            if (trade_pnl < -1000.0) {  // Significant loss threshold
                auto now = std::chrono::high_resolution_clock::now();
                pnl_data.cooldown_until = now + limits.loss_cooldown;
            }
        }
    }
    
    // Reset daily counters (call at start of each trading day)
    void reset_daily_counters() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& [strategy, rate_data] : rate_limiting_) {
            rate_data.daily_orders = 0;
            // Clear order times queue
            while (!rate_data.order_times.empty()) {
                rate_data.order_times.pop();
            }
        }
        
        for (auto& [strategy, pnl_data] : strategy_pnl_) {
            pnl_data.daily_pnl = 0.0;
        }
    }
    
    // Get current positions
    std::unordered_map<std::pair<StrategyId, InstrumentId>, Position, PairHash> get_positions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return positions_;
    }
    
    // Get strategy P&L
    Price get_strategy_pnl(const StrategyId& strategy) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = strategy_pnl_.find(strategy);
        return (it != strategy_pnl_.end()) ? it->second.total_pnl : 0.0;
    }
    
    // Get portfolio statistics
    struct PortfolioStats {
        Price total_pnl = 0.0;
        Price total_exposure = 0.0;
        size_t active_positions = 0;
        Price max_drawdown = 0.0;
    };
    
    PortfolioStats get_portfolio_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        PortfolioStats stats;
        
        for (const auto& [strategy, pnl_data] : strategy_pnl_) {
            stats.total_pnl += pnl_data.total_pnl;
        }
        
        for (const auto& [key, exposure] : exposures_) {
            stats.total_exposure += std::abs(exposure);
        }
        
        for (const auto& [key, position] : positions_) {
            if (position.quantity != 0) {
                stats.active_positions++;
            }
        }
        
        return stats;
    }
    
private:
    struct RateLimitingData {
        std::queue<Timestamp> order_times;
        uint32_t daily_orders = 0;
    };
    
    struct PnLData {
        Price daily_pnl = 0.0;
        Price total_pnl = 0.0;
        Timestamp cooldown_until;
    };
    
    const RiskLimits& get_limits(const StrategyId& strategy) const {
        auto it = strategy_limits_.find(strategy);
        return (it != strategy_limits_.end()) ? it->second : limits_;
    }
    
    Price calculate_trade_pnl(const Fill& fill, const Position& position) const {
        // Simplified P&L calculation
        // In reality, would need more sophisticated accounting
        return -fill.commission;  // For now, just commission cost
    }
    
    mutable std::mutex mutex_;
    RiskLimits limits_;
    std::unordered_map<StrategyId, RiskLimits> strategy_limits_;
    
    // Position tracking
    std::unordered_map<std::pair<StrategyId, InstrumentId>, Position, PairHash> positions_;
    std::unordered_map<std::pair<StrategyId, InstrumentId>, Price, PairHash> exposures_;
    
    // Rate limiting
    std::unordered_map<StrategyId, RateLimitingData> rate_limiting_;
    
    // P&L tracking
    std::unordered_map<StrategyId, PnLData> strategy_pnl_;
};

} // namespace backtest
