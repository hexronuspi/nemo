#include "strategy/strategy_base.h"
#include <iostream>
#include <numeric>
#include <algorithm>

namespace backtest {

SMAStrategy::PriceMode SMAStrategy::price_mode_from_string(const std::string& s) {
    if (s == "close") return PriceMode::CLOSE;
    if (s == "open") return PriceMode::OPEN;
    if (s == "high") return PriceMode::HIGH;
    if (s == "low") return PriceMode::LOW;
    if (s == "hlc3") return PriceMode::HLC3;
    if (s == "ohlc4") return PriceMode::OHLC4;
    return PriceMode::CLOSE;
}

static Price get_price_from_columns(const MarketDataTick& tick, SMAStrategy::PriceMode mode, const std::unordered_map<std::string, std::string>& cols) {
    // For demo, map to tick fields by name
    if (mode == SMAStrategy::PriceMode::CLOSE && cols.count("close")) return tick.last_price;
    if (mode == SMAStrategy::PriceMode::OPEN && cols.count("open")) return tick.bid_price;
    if (mode == SMAStrategy::PriceMode::HIGH && cols.count("high")) return tick.ask_price;
    if (mode == SMAStrategy::PriceMode::LOW && cols.count("low")) return tick.bid_price;
    if (mode == SMAStrategy::PriceMode::HLC3) return (tick.ask_price + tick.bid_price + tick.last_price) / 3.0;
    if (mode == SMAStrategy::PriceMode::OHLC4) return (tick.ask_price + tick.bid_price + tick.last_price + tick.bid_price) / 4.0;
    return tick.last_price;
}

void SMAStrategy::on_market_data(const MarketEvent& event) {
    const auto& tick = event.tick();
    auto& hist = price_histories_[tick.instrument];
    Price price = get_price_from_columns(tick, price_mode_, price_columns_);
    hist.prices.push_back(price);
    if (hist.prices.size() > static_cast<size_t>(long_period_)) hist.prices.erase(hist.prices.begin());
    if (hist.prices.size() < static_cast<size_t>(long_period_)) return;
    Price short_sma = std::accumulate(hist.prices.end()-short_period_, hist.prices.end(), 0.0) / short_period_;
    Price long_sma = std::accumulate(hist.prices.begin(), hist.prices.end(), 0.0) / long_period_;
    if (!hist.has_signal && short_sma > long_sma) {
        execute_order(tick.instrument, Side::BUY, price, 1);
        hist.has_signal = true;
    } else if (hist.has_signal && short_sma < long_sma) {
        execute_order(tick.instrument, Side::SELL, price, 1);
        hist.has_signal = false;
    }
}
void SMAStrategy::initialize() {}
void SMAStrategy::on_fill(const FillEvent& event) {}

// --- MeanReversionStrategy ---
void MeanReversionStrategy::initialize() {}
void MeanReversionStrategy::on_market_data(const MarketEvent& event) {}
void MeanReversionStrategy::on_fill(const FillEvent& event) {}

// --- MomentumStrategy ---
void MomentumStrategy::initialize() {}
void MomentumStrategy::on_market_data(const MarketEvent& event) {}
void MomentumStrategy::on_fill(const FillEvent& event) {}

void StrategyBase::execute_order(const InstrumentId& instrument, Side side, Price price, Volume qty) const {
    static OrderId next_id = 1;
    Order order(next_id++, instrument, strategy_id_, side, OrderType::MARKET, price, qty);
    order.status = OrderStatus::FILLED;
    Logger::get().info("strategy", "Order executed: " + instrument + (side == Side::BUY ? " BUY " : " SELL ") + std::to_string(qty) + " @ " + std::to_string(price));
    auto& pos = const_cast<std::unordered_map<InstrumentId, Position>&>(positions_)[instrument];
    if (side == Side::BUY) {
        pos.quantity += qty;
        pos.average_price = price;
    } else {
        pos.quantity -= qty;
        pos.average_price = price;
    }
    const_cast<size_t&>(trade_count_)++;
}

} // namespace backtest
