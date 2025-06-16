#pragma once

#include "core/events.h"
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace backtest {

// Event handler function type
using EventHandler = std::function<void(const Event&)>;

// Subscription handle for unsubscribing
using SubscriptionHandle = size_t;

class EventBus {
public:
    EventBus() : running_(false), next_handle_(1) {}
    
    ~EventBus() {
        stop();
    }
    
    // Subscribe to specific event type
    template<typename EventT>
    SubscriptionHandle subscribe(std::function<void(const EventT&)> handler) {
        static_assert(std::is_base_of_v<Event, EventT>, "EventT must derive from Event");
        
        std::lock_guard<std::mutex> lock(mutex_);
        auto handle = next_handle_++;
        
        EventHandler wrapper = [handler](const Event& event) {
            if (const auto* typed_event = dynamic_cast<const EventT*>(&event)) {
                handler(*typed_event);
            }
        };
        
        subscribers_[EventT::type_id()].emplace_back(handle, std::move(wrapper));
        handle_to_type_[handle] = EventT::type_id();
        
        return handle;
    }
    
    // Subscribe to all events
    SubscriptionHandle subscribe_all(EventHandler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto handle = next_handle_++;
        
        all_subscribers_.emplace_back(handle, std::move(handler));
        handle_to_type_[handle] = static_cast<uint8_t>(EventType::MARKET_DATA); // Dummy type
        
        return handle;
    }
    
    // Unsubscribe using handle
    void unsubscribe(SubscriptionHandle handle) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = handle_to_type_.find(handle);
        if (it == handle_to_type_.end()) {
            return;
        }
        
        // Remove from specific type subscribers
        auto& type_subscribers = subscribers_[it->second];
        type_subscribers.erase(
            std::remove_if(type_subscribers.begin(), type_subscribers.end(),
                          [handle](const auto& pair) { return pair.first == handle; }),
            type_subscribers.end());
        
        // Remove from all subscribers
        all_subscribers_.erase(
            std::remove_if(all_subscribers_.begin(), all_subscribers_.end(),
                          [handle](const auto& pair) { return pair.first == handle; }),
            all_subscribers_.end());
        
        handle_to_type_.erase(it);
    }
    
    // Publish event (async)
    void publish(EventPtr event) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            event_queue_.push(std::move(event));
        }
        cv_.notify_one();
    }
    
    // Publish event (sync)
    void publish_sync(const Event& event) {
        dispatch_event(event);
    }
    
    // Start event processing thread
    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return;
        }
        
        running_ = true;
        worker_thread_ = std::thread(&EventBus::process_events, this);
    }
    
    // Stop event processing
    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                return;
            }
            running_ = false;
        }
        
        cv_.notify_all();
        
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }
    
    // Process all pending events synchronously
    void process_pending() {
        std::queue<EventPtr> local_queue;
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            local_queue.swap(event_queue_);
        }
        
        while (!local_queue.empty()) {
            auto event = std::move(local_queue.front());
            local_queue.pop();
            dispatch_event(*event);
        }
    }
    
    // Get queue size
    size_t queue_size() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return event_queue_.size();
    }
    
private:
    void process_events() {
        while (true) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            cv_.wait(lock, [this] { return !event_queue_.empty() || !running_; });
            
            if (!running_ && event_queue_.empty()) {
                break;
            }
            
            if (event_queue_.empty()) {
                continue;
            }
            
            auto event = std::move(event_queue_.front());
            event_queue_.pop();
            lock.unlock();
            
            dispatch_event(*event);
        }
    }
    
    void dispatch_event(const Event& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Dispatch to type-specific subscribers
        auto event_type = static_cast<uint8_t>(event.type());
        auto it = subscribers_.find(event_type);
        if (it != subscribers_.end()) {
            for (const auto& [handle, handler] : it->second) {
                try {
                    handler(event);
                } catch (const std::exception& e) {
                    // Log error but continue processing
                    // TODO: Add proper error logging
                }
            }
        }
        
        // Dispatch to all-event subscribers
        for (const auto& [handle, handler] : all_subscribers_) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                // Log error but continue processing
                // TODO: Add proper error logging
            }
        }
    }
    
    mutable std::mutex mutex_;
    mutable std::mutex queue_mutex_;
    std::condition_variable cv_;
    
    std::unordered_map<uint8_t, std::vector<std::pair<SubscriptionHandle, EventHandler>>> subscribers_;
    std::vector<std::pair<SubscriptionHandle, EventHandler>> all_subscribers_;
    std::unordered_map<SubscriptionHandle, uint8_t> handle_to_type_;
    
    std::queue<EventPtr> event_queue_;
    std::thread worker_thread_;
    std::atomic<bool> running_;
    SubscriptionHandle next_handle_;
};

// Global event bus instance
class GlobalEventBus {
public:
    static EventBus& instance() {
        static EventBus instance;
        return instance;
    }
    
private:
    GlobalEventBus() = default;
};

} // namespace backtest
