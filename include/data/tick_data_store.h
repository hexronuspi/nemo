#pragma once

#include "utils/types.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <span>
#include <numeric>

namespace backtest {

// High-performance columnar storage for tick data
class TickDataStore {
public:
    struct TickData {
        std::vector<Timestamp> timestamps;
        std::vector<Price> bid_prices;
        std::vector<Price> ask_prices;
        std::vector<Volume> bid_sizes;
        std::vector<Volume> ask_sizes;
        std::vector<Price> last_prices;
        std::vector<Volume> volumes;
        std::vector<double> open;
        std::vector<double> high;
        std::vector<double> low;
        std::vector<double> close;
        std::vector<std::string> date;
        
        void reserve(size_t capacity) {
            timestamps.reserve(capacity);
            bid_prices.reserve(capacity);
            ask_prices.reserve(capacity);
            bid_sizes.reserve(capacity);
            ask_sizes.reserve(capacity);
            last_prices.reserve(capacity);
            volumes.reserve(capacity);
            open.reserve(capacity);
            high.reserve(capacity);
            low.reserve(capacity);
            close.reserve(capacity);
            date.reserve(capacity);
        }
        
        void clear() {
            timestamps.clear();
            bid_prices.clear();
            ask_prices.clear();
            bid_sizes.clear();
            ask_sizes.clear();
            last_prices.clear();
            volumes.clear();
            open.clear();
            high.clear();
            low.clear();
            close.clear();
            date.clear();
        }
        
        size_t size() const { return timestamps.size(); }
        
        MarketDataTick get_tick(size_t index) const {
            return MarketDataTick{
                timestamps[index],
                "", // instrument will be set by caller
                bid_prices[index],
                ask_prices[index],
                bid_sizes[index],
                ask_sizes[index],
                last_prices[index],
                volumes[index],
                open[index],
                high[index],
                low[index],
                close[index],
                date[index]
            };
        }
        
        void add_tick(const MarketDataTick& tick) {
            timestamps.push_back(tick.timestamp);
            bid_prices.push_back(tick.bid_price);
            ask_prices.push_back(tick.ask_price);
            bid_sizes.push_back(tick.bid_size);
            ask_sizes.push_back(tick.ask_size);
            last_prices.push_back(tick.last_price);
            volumes.push_back(tick.volume);
            open.push_back(tick.open);
            high.push_back(tick.high);
            low.push_back(tick.low);
            close.push_back(tick.close);
            date.push_back(tick.date);
        }
    };
    
    // Add tick data for instrument
    void add_tick(const InstrumentId& instrument, const MarketDataTick& tick) {
        data_[instrument].add_tick(tick);
    }
    
    // Add multiple ticks
    void add_ticks(const InstrumentId& instrument, const std::vector<MarketDataTick>& ticks) {
        auto& instrument_data = data_[instrument];
        instrument_data.reserve(instrument_data.size() + ticks.size());
        
        for (const auto& tick : ticks) {
            instrument_data.add_tick(tick);
        }
    }
    
    // Get all ticks for instrument
    const TickData* get_ticks(const InstrumentId& instrument) const {
        auto it = data_.find(instrument);
        return (it != data_.end()) ? &it->second : nullptr;
    }
    
    // Get tick range by time
    std::vector<MarketDataTick> get_ticks_range(const InstrumentId& instrument,
                                               Timestamp start_time,
                                               Timestamp end_time) const {
        auto it = data_.find(instrument);
        if (it == data_.end()) {
            return {};
        }
        
        const auto& ticks = it->second;
        std::vector<MarketDataTick> result;
        
        for (size_t i = 0; i < ticks.size(); ++i) {
            if (ticks.timestamps[i] >= start_time && ticks.timestamps[i] <= end_time) {
                auto tick = ticks.get_tick(i);
                tick.instrument = instrument;
                result.push_back(tick);
            }
        }
        
        return result;
    }
    
    // Get tick at specific index
    std::optional<MarketDataTick> get_tick_at(const InstrumentId& instrument, size_t index) const {
        auto it = data_.find(instrument);
        if (it == data_.end() || index >= it->second.size()) {
            return std::nullopt;
        }
        
        auto tick = it->second.get_tick(index);
        tick.instrument = instrument;
        return tick;
    }
    
    // Get number of ticks for instrument
    size_t size(const InstrumentId& instrument) const {
        auto it = data_.find(instrument);
        return (it != data_.end()) ? it->second.size() : 0;
    }
    
    // Get all instruments
    std::vector<InstrumentId> get_instruments() const {
        std::vector<InstrumentId> instruments;
        instruments.reserve(data_.size());
        
        for (const auto& [instrument, _] : data_) {
            instruments.push_back(instrument);
        }
        
        return instruments;
    }
    
    // Clear all data
    void clear() {
        data_.clear();
    }
    
    // Clear data for specific instrument
    void clear(const InstrumentId& instrument) {
        auto it = data_.find(instrument);
        if (it != data_.end()) {
            it->second.clear();
        }
    }
    
    // Sort ticks by timestamp for each instrument
    void sort_by_timestamp() {
        for (auto& [instrument, ticks] : data_) {
            // Create index vector for sorting
            std::vector<size_t> indices(ticks.size());
            std::iota(indices.begin(), indices.end(), 0);
            
            // Sort indices by timestamp
            std::sort(indices.begin(), indices.end(), 
                     [&ticks](size_t a, size_t b) {
                         return ticks.timestamps[a] < ticks.timestamps[b];
                     });
            
            // Reorder all vectors
            reorder_vectors(ticks, indices);
        }
    }
    
    // Get memory usage in bytes
    size_t memory_usage() const {
        size_t total = 0;
        for (const auto& [instrument, ticks] : data_) {
            total += sizeof(Timestamp) * ticks.timestamps.capacity();
            total += sizeof(Price) * ticks.bid_prices.capacity();
            total += sizeof(Price) * ticks.ask_prices.capacity();
            total += sizeof(Volume) * ticks.bid_sizes.capacity();
            total += sizeof(Volume) * ticks.ask_sizes.capacity();
            total += sizeof(Price) * ticks.last_prices.capacity();
            total += sizeof(Volume) * ticks.volumes.capacity();
            total += sizeof(double) * ticks.open.capacity();
            total += sizeof(double) * ticks.high.capacity();
            total += sizeof(double) * ticks.low.capacity();
            total += sizeof(double) * ticks.close.capacity();
            total += sizeof(std::string) * ticks.date.capacity();
        }
        return total;
    }
    
    // Statistics
    struct Statistics {
        size_t total_ticks = 0;
        size_t total_instruments = 0;
        Timestamp earliest_time;
        Timestamp latest_time;
        size_t memory_usage_bytes = 0;
    };
    
    Statistics get_statistics() const {
        Statistics stats;
        stats.total_instruments = data_.size();
        stats.memory_usage_bytes = memory_usage();
        
        if (data_.empty()) {
            stats.earliest_time = stats.latest_time = std::chrono::high_resolution_clock::now();
            return stats;
        }
        
        stats.earliest_time = std::chrono::high_resolution_clock::time_point::max();
        stats.latest_time = std::chrono::high_resolution_clock::time_point::min();
        
        for (const auto& [instrument, ticks] : data_) {
            stats.total_ticks += ticks.size();
            
            if (!ticks.timestamps.empty()) {
                auto [min_it, max_it] = std::minmax_element(
                    ticks.timestamps.begin(), ticks.timestamps.end());
                stats.earliest_time = std::min(stats.earliest_time, *min_it);
                stats.latest_time = std::max(stats.latest_time, *max_it);
            }
        }
        
        return stats;
    }
    
    // Return all ticks for all instruments (for event loop)
    std::unordered_map<InstrumentId, std::vector<MarketDataTick>> get_all_ticks() const {
        std::unordered_map<InstrumentId, std::vector<MarketDataTick>> all;
        for (const auto& [inst, data] : data_) {
            std::vector<MarketDataTick> ticks;
            for (size_t i = 0; i < data.size(); ++i) {
                auto tick = data.get_tick(i);
                tick.instrument = inst;
                ticks.push_back(tick);
            }
            all[inst] = std::move(ticks);
        }
        return all;
    }
    
private:
    void reorder_vectors(TickData& ticks, const std::vector<size_t>& indices) {
        auto reorder = [&indices](auto& vec) {
            using T = typename std::decay_t<decltype(vec)>::value_type;
            std::vector<T> temp;
            temp.reserve(vec.size());
            
            for (size_t idx : indices) {
                temp.push_back(std::move(vec[idx]));
            }
            
            vec = std::move(temp);
        };
        
        reorder(ticks.timestamps);
        reorder(ticks.bid_prices);
        reorder(ticks.ask_prices);
        reorder(ticks.bid_sizes);
        reorder(ticks.ask_sizes);
        reorder(ticks.last_prices);
        reorder(ticks.volumes);
        reorder(ticks.open);
        reorder(ticks.high);
        reorder(ticks.low);
        reorder(ticks.close);
        reorder(ticks.date);
    }
    
    std::unordered_map<InstrumentId, TickData> data_;
};

} // namespace backtest
