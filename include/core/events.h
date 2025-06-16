#pragma once

#include "utils/types.h"
#include <memory>
#include <variant>
#include <vector>

namespace backtest {

// Base event class
class Event {
public:
    explicit Event(EventType type, Timestamp timestamp = std::chrono::high_resolution_clock::now())
        : type_(type), timestamp_(timestamp) {}
    
    virtual ~Event() = default;
    
    EventType type() const { return type_; }
    Timestamp timestamp() const { return timestamp_; }
    
private:
    EventType type_;
    Timestamp timestamp_;
};

// Market data event
class MarketEvent : public Event {
public:
    explicit MarketEvent(const MarketDataTick& tick)
        : Event(EventType::MARKET_DATA, tick.timestamp), tick_(tick) {}
    
    const MarketDataTick& tick() const { return tick_; }
    
private:
    MarketDataTick tick_;
};

// Signal event for trading signals
class SignalEvent : public Event {
public:
    enum class SignalType : uint8_t {
        BUY = 0,
        SELL = 1,
        HOLD = 2,
        CLOSE = 3
    };
    
    SignalEvent(const InstrumentId& instrument, const StrategyId& strategy,
                SignalType signal_type, Price strength = 1.0, 
                Timestamp timestamp = std::chrono::high_resolution_clock::now())
        : Event(EventType::SIGNAL, timestamp), instrument_(instrument),
          strategy_(strategy), signal_type_(signal_type), strength_(strength) {}
    
    const InstrumentId& instrument() const { return instrument_; }
    const StrategyId& strategy() const { return strategy_; }
    SignalType signal_type() const { return signal_type_; }
    Price strength() const { return strength_; }
    
private:
    InstrumentId instrument_;
    StrategyId strategy_;
    SignalType signal_type_;
    Price strength_;
};

// Order event
class OrderEvent : public Event {
public:
    explicit OrderEvent(const Order& order)
        : Event(EventType::ORDER, order.timestamp), order_(order) {}
    
    const Order& order() const { return order_; }
    Order& order() { return order_; }
    
private:
    Order order_;
};

// Fill event
class FillEvent : public Event {
public:
    explicit FillEvent(const Fill& fill)
        : Event(EventType::FILL, fill.timestamp), fill_(fill) {}
    
    const Fill& fill() const { return fill_; }
    
private:
    Fill fill_;
};

// Risk event for risk management
class RiskEvent : public Event {
public:
    enum class RiskType : uint8_t {
        POSITION_LIMIT = 0,
        LOSS_LIMIT = 1,
        EXPOSURE_LIMIT = 2,
        COOLDOWN = 3
    };
    
    RiskEvent(RiskType risk_type, const StrategyId& strategy, 
              const std::string& message,
              Timestamp timestamp = std::chrono::high_resolution_clock::now())
        : Event(EventType::RISK, timestamp), risk_type_(risk_type),
          strategy_(strategy), message_(message) {}
    
    RiskType risk_type() const { return risk_type_; }
    const StrategyId& strategy() const { return strategy_; }
    const std::string& message() const { return message_; }
    
private:
    RiskType risk_type_;
    StrategyId strategy_;
    std::string message_;
};

// Timer event for scheduled operations
class TimerEvent : public Event {
public:
    explicit TimerEvent(const std::string& timer_id,
                       Timestamp timestamp = std::chrono::high_resolution_clock::now())
        : Event(EventType::TIMER, timestamp), timer_id_(timer_id) {}
    
    const std::string& timer_id() const { return timer_id_; }
    
private:
    std::string timer_id_;
};

// Event pointer type
using EventPtr = std::unique_ptr<Event>;

// Event variant for type-safe handling
using EventVariant = std::variant<
    std::unique_ptr<MarketEvent>,
    std::unique_ptr<SignalEvent>,
    std::unique_ptr<OrderEvent>,
    std::unique_ptr<FillEvent>,
    std::unique_ptr<RiskEvent>,
    std::unique_ptr<TimerEvent>
>;

} // namespace backtest
