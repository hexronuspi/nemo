// Path: c:\Users\aruna\Desktop\backtest\include\utils\types.h
#pragma once

#include <chrono>
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <variant>
#include <string_view>
#include <functional>

namespace backtest {

// Time types with nanosecond precision
using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
using Duration = std::chrono::nanoseconds;

// Fundamental types
using Price = double;
using Volume = uint64_t;
using OrderId = uint64_t;
using StrategyId = std::string;
using InstrumentId = std::string;
using ExchangeId = std::string;

// Trading types
enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderType : uint8_t {
    MARKET = 0,
    LIMIT = 1,
    STOP = 2,
    STOP_LIMIT = 3
};

enum class OrderStatus : uint8_t {
    PENDING = 0,
    PARTIAL_FILL = 1,
    FILLED = 2,
    CANCELLED = 3,
    REJECTED = 4
};

enum class EventType : uint8_t {
    MARKET_DATA = 0,
    SIGNAL = 1,
    ORDER = 2,
    FILL = 3,
    RISK = 4,
    TIMER = 5
};

struct MarketDataTick {
    Timestamp timestamp;
    InstrumentId instrument;
    Price bid_price;
    Price ask_price;
    Volume bid_size;
    Volume ask_size;
    Price last_price;
    Volume volume;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    std::string date;
    
    MarketDataTick() = default;
    // Only one constructor for all fields:
    MarketDataTick(Timestamp ts, const InstrumentId& inst,
                   Price bid, Price ask, Volume bid_vol, Volume ask_vol,
                   Price last, Volume vol,
                   double open_, double high_, double low_, double close_, const std::string& date_)
        : timestamp(ts), instrument(inst), bid_price(bid), ask_price(ask),
          bid_size(bid_vol), ask_size(ask_vol), last_price(last), volume(vol),
          open(open_), high(high_), low(low_), close(close_), date(date_) {}
};

struct Order {
    OrderId id;
    Timestamp timestamp;
    InstrumentId instrument;
    StrategyId strategy;
    Side side;
    OrderType type;
    Price price;
    Volume quantity;
    Volume filled_quantity = 0;
    OrderStatus status = OrderStatus::PENDING;
    std::optional<Price> stop_price;
    
    Order() = default;
    Order(OrderId order_id, const InstrumentId& inst, const StrategyId& strat,
          Side order_side, OrderType order_type, Price order_price, Volume qty)
        : id(order_id), instrument(inst), strategy(strat), side(order_side),
          type(order_type), price(order_price), quantity(qty) {}
};

struct Fill {
    OrderId order_id;
    Timestamp timestamp;
    InstrumentId instrument;
    StrategyId strategy;
    Side side;
    Price price;
    Volume quantity;
    Price commission;
    
    Fill() = default;
    Fill(OrderId oid, Timestamp ts, const InstrumentId& inst, 
         const StrategyId& strat, Side fill_side, Price fill_price, 
         Volume qty, Price comm = 0.0)
        : order_id(oid), timestamp(ts), instrument(inst), strategy(strat),
          side(fill_side), price(fill_price), quantity(qty), commission(comm) {}
};

struct Position {
    InstrumentId instrument;
    StrategyId strategy;
    Volume quantity = 0;  // Positive for long, negative for short
    Price average_price = 0.0;
    Price unrealized_pnl = 0.0;
    Price realized_pnl = 0.0;
    
    Position() = default;
    Position(const InstrumentId& inst, const StrategyId& strat)
        : instrument(inst), strategy(strat) {}
};

} // namespace backtest
