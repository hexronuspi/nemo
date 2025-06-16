#pragma once
#include <cmath>
#include <algorithm>

#include "utils/types.h"
#include <unordered_map>
#include <functional>

namespace backtest {

// Commission structure for different exchanges/brokers
struct CommissionStructure {
    Price maker_fee_rate = 0.0;        // Fee for providing liquidity (negative = rebate)
    Price taker_fee_rate = 0.001;      // Fee for taking liquidity
    Price fixed_fee = 0.0;             // Fixed fee per trade
    Price min_commission = 0.0;        // Minimum commission
    Price max_commission = 1000000.0;  // Maximum commission
    
    Price calculate_commission(Volume quantity, Price price, bool is_maker) const {
        Price rate = is_maker ? maker_fee_rate : taker_fee_rate;
        Price commission = quantity * price * rate + fixed_fee;
        return std::clamp(commission, min_commission, max_commission);
    }
};

// Slippage model
class SlippageModel {
public:
    virtual ~SlippageModel() = default;
    virtual Price calculate_slippage(const InstrumentId& instrument, Side side, 
                                   Volume quantity, Price reference_price,
                                   Volume avg_daily_volume) const = 0;
};

// Linear slippage model: slippage = base_rate + impact_rate * (quantity / avg_volume)
class LinearSlippageModel : public SlippageModel {
public:
    LinearSlippageModel(Price base_rate = 0.0001, Price impact_rate = 0.01)
        : base_rate_(base_rate), impact_rate_(impact_rate) {}
    
    Price calculate_slippage(const InstrumentId& instrument, Side side, 
                           Volume quantity, Price reference_price,
                           Volume avg_daily_volume) const override {
        if (avg_daily_volume == 0) {
            return base_rate_ * reference_price;
        }
        
        double volume_ratio = static_cast<double>(quantity) / avg_daily_volume;
        Price slippage_rate = base_rate_ + impact_rate_ * volume_ratio;
        
        // Slippage is always negative for performance (cost)
        return -std::abs(slippage_rate * reference_price);
    }
    
private:
    Price base_rate_;
    Price impact_rate_;
};

// Square-root slippage model: more realistic for larger orders
class SqrtSlippageModel : public SlippageModel {
public:
    SqrtSlippageModel(Price base_rate = 0.0001, Price impact_coefficient = 0.1)
        : base_rate_(base_rate), impact_coefficient_(impact_coefficient) {}
    
    Price calculate_slippage(const InstrumentId& instrument, Side side, 
                           Volume quantity, Price reference_price,
                           Volume avg_daily_volume) const override {
        if (avg_daily_volume == 0) {
            return base_rate_ * reference_price;
        }
        
        double volume_ratio = static_cast<double>(quantity) / avg_daily_volume;
        Price slippage_rate = base_rate_ + impact_coefficient_ * std::sqrt(volume_ratio);
        
        return -std::abs(slippage_rate * reference_price);
    }
    
private:
    Price base_rate_;
    Price impact_coefficient_;
};

// Comprehensive cost model
class CostModel {
public:
    CostModel() : slippage_model_(std::make_unique<LinearSlippageModel>()) {}
    
    // Set commission structure for exchange/instrument
    void set_commission_structure(const ExchangeId& exchange, 
                                 const CommissionStructure& structure) {
        commission_structures_[exchange] = structure;
    }
    
    // Set commission structure for specific instrument
    void set_instrument_commission(const InstrumentId& instrument,
                                  const CommissionStructure& structure) {
        instrument_commissions_[instrument] = structure;
    }
    
    // Set slippage model
    void set_slippage_model(std::unique_ptr<SlippageModel> model) {
        slippage_model_ = std::move(model);
    }
    
    // Set average daily volume for slippage calculation
    void set_avg_daily_volume(const InstrumentId& instrument, Volume volume) {
        avg_daily_volumes_[instrument] = volume;
    }
    
    // Calculate total transaction cost
    struct TransactionCost {
        Price commission = 0.0;
        Price slippage = 0.0;
        Price total_cost = 0.0;
        
        TransactionCost() = default;
        TransactionCost(Price comm, Price slip) : commission(comm), slippage(slip) {
            total_cost = commission + slippage;
        }
    };
    
    TransactionCost calculate_cost(const InstrumentId& instrument,
                                  const ExchangeId& exchange,
                                  Side side, Volume quantity, Price price,
                                  bool is_aggressive = true) const {
        // Calculate commission
        Price commission = calculate_commission(instrument, exchange, quantity, price, !is_aggressive);
        
        // Calculate slippage
        Price slippage = 0.0;
        if (slippage_model_) {
            auto it = avg_daily_volumes_.find(instrument);
            Volume avg_vol = (it != avg_daily_volumes_.end()) ? it->second : 1000000;  // Default
            slippage = slippage_model_->calculate_slippage(instrument, side, quantity, price, avg_vol);
        }
        
        return TransactionCost(commission, slippage);
    }
    
    // Calculate cost for a fill
    TransactionCost calculate_fill_cost(const Fill& fill, const ExchangeId& exchange = "default") const {
        return calculate_cost(fill.instrument, exchange, fill.side, 
                            fill.quantity, fill.price, true);
    }
    
private:
    Price calculate_commission(const InstrumentId& instrument, const ExchangeId& exchange,
                             Volume quantity, Price price, bool is_maker) const {
        // Check instrument-specific commission first
        auto inst_it = instrument_commissions_.find(instrument);
        if (inst_it != instrument_commissions_.end()) {
            return inst_it->second.calculate_commission(quantity, price, is_maker);
        }
        
        // Fall back to exchange commission
        auto exch_it = commission_structures_.find(exchange);
        if (exch_it != commission_structures_.end()) {
            return exch_it->second.calculate_commission(quantity, price, is_maker);
        }
        
        // Default commission structure
        static const CommissionStructure default_structure;
        return default_structure.calculate_commission(quantity, price, is_maker);
    }
    
    std::unordered_map<ExchangeId, CommissionStructure> commission_structures_;
    std::unordered_map<InstrumentId, CommissionStructure> instrument_commissions_;
    std::unordered_map<InstrumentId, Volume> avg_daily_volumes_;
    std::unique_ptr<SlippageModel> slippage_model_;
};

// Predefined cost models for common exchanges
namespace CostModels {
    
    // US Equity market (typical retail broker)
    inline CostModel create_us_equity_model() {
        CostModel model;
        
        CommissionStructure us_equity;
        us_equity.taker_fee_rate = 0.0;
        us_equity.maker_fee_rate = 0.0;
        us_equity.fixed_fee = 0.0;  // Zero commission
        
        model.set_commission_structure("us_equity", us_equity);
        model.set_slippage_model(std::make_unique<LinearSlippageModel>(0.0001, 0.01));
        
        return model;
    }
    
    // Crypto exchange (Binance-like)
    inline CostModel create_crypto_model() {
        CostModel model;
        
        CommissionStructure crypto;
        crypto.maker_fee_rate = 0.001;   // 0.1%
        crypto.taker_fee_rate = 0.001;   // 0.1%
        
        model.set_commission_structure("crypto", crypto);
        model.set_slippage_model(std::make_unique<SqrtSlippageModel>(0.0005, 0.1));
        
        return model;
    }
    
    // Forex market
    inline CostModel create_forex_model() {
        CostModel model;
        
        CommissionStructure forex;
        forex.maker_fee_rate = 0.0;
        forex.taker_fee_rate = 0.0;
        forex.fixed_fee = 0.0;  // Spread-only costs
        
        model.set_commission_structure("forex", forex);
        model.set_slippage_model(std::make_unique<LinearSlippageModel>(0.00005, 0.005));
        
        return model;
    }
}

} // namespace backtest
