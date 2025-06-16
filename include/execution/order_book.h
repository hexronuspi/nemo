#pragma once

#include "utils/types.h"
#include <map>
#include <queue>
#include <memory>
#include <functional>

namespace backtest {

// Order book level
struct BookLevel {
    Price price;
    Volume total_volume;
    std::queue<std::pair<OrderId, Volume>> orders; // FIFO for price-time priority
    
    BookLevel() : price(0.0), total_volume(0) {}
    BookLevel(Price p) : price(p), total_volume(0) {}
    
    void add_order(OrderId id, Volume volume) {
        orders.emplace(id, volume);
        total_volume += volume;
    }
    
    Volume remove_order(OrderId id, Volume volume) {
        // Simplified - in reality would need to track order positions
        if (total_volume >= volume) {
            total_volume -= volume;
            return volume;
        }
        Volume removed = total_volume;
        total_volume = 0;
        return removed;
    }
};

// Limit Order Book implementation
class OrderBook {
public:
    enum class MatchingAlgorithm {
        PRICE_TIME,      // Price-time priority (FIFO)
        PRO_RATA,        // Pro-rata allocation
        PRICE_SIZE_TIME  // Price-size-time priority
    };
    
    explicit OrderBook(const InstrumentId& instrument, 
                      MatchingAlgorithm algo = MatchingAlgorithm::PRICE_TIME)
        : instrument_(instrument), matching_algo_(algo) {}
    
    // Add order to book
    void add_order(const Order& order) {
        if (order.side == Side::BUY) {
            bids_[order.price].add_order(order.id, order.quantity);
        } else {
            asks_[order.price].add_order(order.id, order.quantity);
        }
    }
    
    // Remove order from book
    void remove_order(OrderId order_id, Side side, Price price, Volume quantity) {
        if (side == Side::BUY) {
            auto it = bids_.find(price);
            if (it != bids_.end()) {
                it->second.remove_order(order_id, quantity);
                if (it->second.total_volume == 0) {
                    bids_.erase(it);
                }
            }
        } else {
            auto it = asks_.find(price);
            if (it != asks_.end()) {
                it->second.remove_order(order_id, quantity);
                if (it->second.total_volume == 0) {
                    asks_.erase(it);
                }
            }
        }
    }
    
    // Execute market order and return fills
    std::vector<Fill> execute_market_order(const Order& order, Timestamp timestamp) {
        std::vector<Fill> fills;
        Volume remaining = order.quantity;
        
        if (order.side == Side::BUY) {
            // Buy at best asks
            auto it = asks_.begin();
            while (it != asks_.end() && remaining > 0) {
                Volume fill_qty = std::min(remaining, it->second.total_volume);
                
                fills.emplace_back(order.id, timestamp, instrument_, order.strategy,
                                 order.side, it->first, fill_qty);
                
                remaining -= fill_qty;
                it->second.total_volume -= fill_qty;
                
                if (it->second.total_volume == 0) {
                    it = asks_.erase(it);
                } else {
                    ++it;
                }
            }
        } else {
            // Sell at best bids
            auto it = bids_.rbegin();
            while (it != bids_.rend() && remaining > 0) {
                Volume fill_qty = std::min(remaining, it->second.total_volume);
                
                fills.emplace_back(order.id, timestamp, instrument_, order.strategy,
                                 order.side, it->first, fill_qty);
                
                remaining -= fill_qty;
                it->second.total_volume -= fill_qty;
                
                if (it->second.total_volume == 0) {
                    // Convert reverse iterator to forward iterator for erase
                    auto forward_it = std::next(it).base();
                    bids_.erase(forward_it);
                    it = bids_.rbegin(); // Reset iterator
                } else {
                    ++it;
                }
            }
        }
        
        return fills;
    }
    
    // Check if limit order can be filled immediately
    std::vector<Fill> execute_limit_order(const Order& order, Timestamp timestamp) {
        std::vector<Fill> fills;
        Volume remaining = order.quantity;
        
        if (order.side == Side::BUY) {
            // Check if we can cross the spread
            auto it = asks_.begin();
            while (it != asks_.end() && it->first <= order.price && remaining > 0) {
                Volume fill_qty = std::min(remaining, it->second.total_volume);
                
                fills.emplace_back(order.id, timestamp, instrument_, order.strategy,
                                 order.side, it->first, fill_qty);
                
                remaining -= fill_qty;
                it->second.total_volume -= fill_qty;
                
                if (it->second.total_volume == 0) {
                    it = asks_.erase(it);
                } else {
                    ++it;
                }
            }
            
            // Add remaining quantity to book
            if (remaining > 0) {
                Order partial_order = order;
                partial_order.quantity = remaining;
                add_order(partial_order);
            }
        } else {
            // Sell side
            auto it = bids_.rbegin();
            while (it != bids_.rend() && it->first >= order.price && remaining > 0) {
                Volume fill_qty = std::min(remaining, it->second.total_volume);
                
                fills.emplace_back(order.id, timestamp, instrument_, order.strategy,
                                 order.side, it->first, fill_qty);
                
                remaining -= fill_qty;
                it->second.total_volume -= fill_qty;
                
                if (it->second.total_volume == 0) {
                    auto forward_it = std::next(it).base();
                    bids_.erase(forward_it);
                    it = bids_.rbegin();
                } else {
                    ++it;
                }
            }
            
            // Add remaining quantity to book
            if (remaining > 0) {
                Order partial_order = order;
                partial_order.quantity = remaining;
                add_order(partial_order);
            }
        }
        
        return fills;
    }
    
    // Get best bid price
    std::optional<Price> best_bid() const {
        if (bids_.empty()) return std::nullopt;
        return bids_.rbegin()->first;
    }
    
    // Get best ask price
    std::optional<Price> best_ask() const {
        if (asks_.empty()) return std::nullopt;
        return asks_.begin()->first;
    }
    
    // Get spread
    std::optional<Price> spread() const {
        auto bid = best_bid();
        auto ask = best_ask();
        if (!bid || !ask) return std::nullopt;
        return *ask - *bid;
    }
    
    // Get mid price
    std::optional<Price> mid_price() const {
        auto bid = best_bid();
        auto ask = best_ask();
        if (!bid || !ask) return std::nullopt;
        return (*bid + *ask) / 2.0;
    }
    
    // Get market depth (L2 data)
    struct DepthLevel {
        Price price;
        Volume volume;
    };
    
    std::vector<DepthLevel> get_bids(size_t levels = 10) const {
        std::vector<DepthLevel> result;
        result.reserve(std::min(levels, bids_.size()));
        
        auto it = bids_.rbegin();
        for (size_t i = 0; i < levels && it != bids_.rend(); ++i, ++it) {
            result.push_back({it->first, it->second.total_volume});
        }
        
        return result;
    }
    
    std::vector<DepthLevel> get_asks(size_t levels = 10) const {
        std::vector<DepthLevel> result;
        result.reserve(std::min(levels, asks_.size()));
        
        auto it = asks_.begin();
        for (size_t i = 0; i < levels && it != asks_.end(); ++i, ++it) {
            result.push_back({it->first, it->second.total_volume});
        }
        
        return result;
    }
    
    // Get total volume at price level
    Volume get_volume_at_price(Side side, Price price) const {
        if (side == Side::BUY) {
            auto it = bids_.find(price);
            return (it != bids_.end()) ? it->second.total_volume : 0;
        } else {
            auto it = asks_.find(price);
            return (it != asks_.end()) ? it->second.total_volume : 0;
        }
    }
    
    // Clear the book
    void clear() {
        bids_.clear();
        asks_.clear();
    }
    
    // Get book statistics
    struct BookStats {
        size_t bid_levels = 0;
        size_t ask_levels = 0;
        Volume total_bid_volume = 0;
        Volume total_ask_volume = 0;
        std::optional<Price> best_bid;
        std::optional<Price> best_ask;
        std::optional<Price> spread;
    };
    
    BookStats get_stats() const {
        BookStats stats;
        stats.bid_levels = bids_.size();
        stats.ask_levels = asks_.size();
        stats.best_bid = best_bid();
        stats.best_ask = best_ask();
        stats.spread = spread();
        
        for (const auto& [price, level] : bids_) {
            stats.total_bid_volume += level.total_volume;
        }
        
        for (const auto& [price, level] : asks_) {
            stats.total_ask_volume += level.total_volume;
        }
        
        return stats;
    }
    
private:
    InstrumentId instrument_;
    MatchingAlgorithm matching_algo_;
    
    // Price levels: price -> BookLevel
    // Bids: highest price first (std::greater for reverse order)
    // Asks: lowest price first (default std::less)
    std::map<Price, BookLevel, std::greater<Price>> bids_;
    std::map<Price, BookLevel> asks_;
};

} // namespace backtest
