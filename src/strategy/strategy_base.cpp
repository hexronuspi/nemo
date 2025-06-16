#include "strategy/strategy_base.h"
#include "strategy/simple_sma_broad.h"
#include <unordered_map>
#include <algorithm>
#include <iostream>

namespace backtest {

namespace StrategyFactory {
std::unique_ptr<StrategyBase> create_sma_strategy(const StrategyId& id, int short_period, int long_period, SMAStrategy::PriceMode price_mode, std::unordered_map<std::string, std::string> price_columns) {
    return std::make_unique<SMAStrategy>(id, short_period, long_period, price_mode, std::move(price_columns));
}
} // namespace StrategyFactory

std::unique_ptr<StrategyBase> StrategyFactory::create_sma_strategy(const StrategyId& id, int short_period, int long_period) {
    return std::make_unique<SMAStrategy>(id, short_period, long_period, SMAStrategy::PriceMode::CLOSE, std::unordered_map<std::string, std::string>{{"close", "close"}});
}
std::unique_ptr<StrategyBase> StrategyFactory::create_mean_reversion_strategy(const StrategyId& id, int lookback, double threshold) {
    return std::make_unique<MeanReversionStrategy>(id, lookback, threshold);
}
std::unique_ptr<StrategyBase> StrategyFactory::create_momentum_strategy(const StrategyId& id, int lookback, double threshold) {
    return std::make_unique<MomentumStrategy>(id, lookback, threshold);
}

} // namespace backtest
