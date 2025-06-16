#pragma once

#include "utils/types.h"
#include <queue>
#include <functional>
#include <mutex>
#include <atomic>
#include <map>
#include <stdexcept>

namespace backtest {

// Event scheduling for time-based operations
struct ScheduledEvent {
    Timestamp execution_time;
    std::function<void()> callback;
    
    bool operator>(const ScheduledEvent& other) const {
        return execution_time > other.execution_time;
    }
};

class SimClock {
public:
    SimClock() : current_time_(std::chrono::high_resolution_clock::now()) {}
    
    // Get current simulation time
    Timestamp now() const { 
        std::lock_guard<std::mutex> lock(mutex_);
        return current_time_; 
    }
    
    // Advance simulation time
    void advance_to(Timestamp new_time) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (new_time < current_time_) {
            throw std::runtime_error("Cannot advance clock backwards");
        }
        current_time_ = new_time;
        process_scheduled_events();
    }
    
    // Advance by duration
    void advance_by(Duration duration) {
        advance_to(now() + duration);
    }
    
    // Schedule a callback for future execution
    void schedule(Timestamp execution_time, std::function<void()> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        scheduled_events_.emplace(ScheduledEvent{execution_time, std::move(callback)});
    }
    
    // Schedule with delay from current time
    void schedule_delay(Duration delay, std::function<void()> callback) {
        schedule(now() + delay, std::move(callback));
    }
    
    // Reset clock to new time
    void reset(Timestamp new_time = std::chrono::high_resolution_clock::now()) {
        std::lock_guard<std::mutex> lock(mutex_);
        current_time_ = new_time;
        // Clear scheduled events
        while (!scheduled_events_.empty()) {
            scheduled_events_.pop();
        }
    }
    
    // Check if there are pending scheduled events
    bool has_pending_events() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return !scheduled_events_.empty();
    }
    
    // Get next scheduled event time
    std::optional<Timestamp> next_event_time() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (scheduled_events_.empty()) {
            return std::nullopt;
        }
        return scheduled_events_.top().execution_time;
    }
    
private:
    void process_scheduled_events() {
        while (!scheduled_events_.empty() && 
               scheduled_events_.top().execution_time <= current_time_) {
            auto event = scheduled_events_.top();
            scheduled_events_.pop();
            
            // Execute callback outside of lock to prevent deadlock
            mutex_.unlock();
            try {
                event.callback();
            } catch (const std::exception& e) {
                // Log error but continue processing
                // TODO: Add proper error logging
            }
            mutex_.lock();
        }
    }
    
    mutable std::mutex mutex_;
    Timestamp current_time_;
    std::priority_queue<ScheduledEvent, std::vector<ScheduledEvent>, std::greater<ScheduledEvent>> scheduled_events_;
};

// Global master clock for synchronization
class MasterClock {
public:
    static MasterClock& instance() {
        static MasterClock instance;
        return instance;
    }
    
    // Register a clock for synchronization
    void register_clock(const std::string& name, std::shared_ptr<SimClock> clock) {
        std::lock_guard<std::mutex> lock(mutex_);
        clocks_[name] = clock;
    }
    
    // Unregister clock
    void unregister_clock(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        clocks_.erase(name);
    }
    
    // Advance all registered clocks to specified time
    void advance_all_to(Timestamp new_time) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, clock] : clocks_) {
            if (auto shared_clock = clock.lock()) {
                shared_clock->advance_to(new_time);
            }
        }
    }
    
    // Get minimum time across all clocks
    Timestamp min_time() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (clocks_.empty()) {
            return std::chrono::high_resolution_clock::now();
        }
        
        Timestamp min_time = std::chrono::high_resolution_clock::time_point::max();
        for (const auto& [name, weak_clock] : clocks_) {
            if (auto clock = weak_clock.lock()) {
                min_time = std::min(min_time, clock->now());
            }
        }
        return min_time;
    }
    
    // Reset all clocks
    void reset_all(Timestamp new_time = std::chrono::high_resolution_clock::now()) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [name, weak_clock] : clocks_) {
            if (auto clock = weak_clock.lock()) {
                clock->reset(new_time);
            }
        }
    }
    
private:
    MasterClock() = default;
    
    mutable std::mutex mutex_;
    std::map<std::string, std::weak_ptr<SimClock>> clocks_;
};

} // namespace backtest
