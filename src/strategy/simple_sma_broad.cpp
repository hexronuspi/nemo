#include "strategy/simple_sma_broad.h"
#include "utils/logging.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <vector>
#include <cmath>
#include <map>
#include <algorithm>
#include <iomanip>

namespace backtest {

// Helper functions for indicators (EMA, RSI, ATR, ADX, etc.)
namespace {
    double ema(const std::vector<double>& data, int period, int idx) {
        if (idx < period - 1) return NAN;
        double k = 2.0 / (period + 1);
        double ema_val = data[idx - period + 1];
        for (int i = idx - period + 2; i <= idx; ++i) {
            ema_val = data[i] * k + ema_val * (1 - k);
        }
        return ema_val;
    }
    double rsi(const std::vector<double>& close, int period, int idx) {
        if (idx < period) return NAN;
        double gain = 0, loss = 0;
        for (int i = idx - period + 1; i <= idx; ++i) {
            double delta = close[i] - close[i - 1];
            if (delta > 0) gain += delta;
            else loss -= delta;
        }
        if (gain + loss == 0) return 50.0;
        double rs = gain / (loss == 0 ? 1e-10 : loss);
        return 100 - (100 / (1 + rs));
    }
    double rolling_mean(const std::vector<double>& data, int period, int idx) {
        if (idx < period - 1) return NAN;
        double sum = 0;
        for (int i = idx - period + 1; i <= idx; ++i) sum += data[i];
        return sum / period;
    }
}

SimpleSMABroadStrategy::SimpleSMABroadStrategy(const StrategyId& id,
                                               int short_ema_param, int long_ema_param, int rsi_period_param, 
                                               double rsi_lb_param, double rsi_ub_param,
                                               int atr_period_param, int adx_period_param, 
                                               double adx_threshold_param, double risk_per_trade_param, 
                                               double initial_capital_param, double slippage_param,
                                               double max_daily_drawdown_param)
    : StrategyBase(id), 
      short_ema(short_ema_param),
      long_ema(long_ema_param),
      rsi_period(rsi_period_param),
      rsi_lb(rsi_lb_param),
      rsi_ub(rsi_ub_param),
      atr_period(atr_period_param),
      adx_period(adx_period_param),
      adx_threshold(adx_threshold_param),
      risk_per_trade(risk_per_trade_param),
      initial_capital(initial_capital_param),
      slippage(slippage_param),
      max_daily_drawdown(max_daily_drawdown_param) {
    equity = initial_capital;
    daily_peak = equity;
    position = 0;
    entry_price = stop_level = tp_level = original_stop_distance = 0.0;
    log_path = "logs/simpleSMABroad_trades.log";
    last_date = "";
    print_count_m = 0; // Initialize debug print counter
}

void SimpleSMABroadStrategy::initialize() {
    trade_logs.clear();
    std::ofstream(log_path, std::ios::trunc); // clear log file
    close.clear(); high.clear(); low.clear(); volume.clear(); datetime.clear();
    // Clear indicator history vectors
    tr_hist_m.clear();
    plus_dm_hist_m.clear();
    minus_dm_hist_m.clear();
    dx_hist_m.clear();
    print_count_m = 0; // Reset debug print counter on initialize
}

void SimpleSMABroadStrategy::on_market_data(const MarketEvent& event) {
    const auto& tick = event.tick();
    // Use correct fields from MarketDataTick
    close.push_back(tick.close);
    high.push_back(tick.high);
    low.push_back(tick.low);
    volume.push_back(tick.volume);
    datetime.push_back(tick.date);
    int idx = close.size() - 1;

    if (print_count_m < 5) {
        std::cout << "[TICK] idx=" << idx
                  << ", date='" << tick.date << "'"
                  << ", open=" << tick.open
                  << ", high=" << tick.high
                  << ", low=" << tick.low
                  << ", close=" << tick.close
                  << ", volume=" << tick.volume << "\n";
        std::cout.flush();
        ++print_count_m;
    }

    // Ensure we have enough data for indicators
    if (idx == 0) { // Not enough data for any indicator that needs a previous value
        // For ATR, we need at least one previous close for hc and lc
        // For ADX, we need at least one previous high and low
        // Initialize history for these cases if needed or just return
        tr_hist_m.push_back(0); // Placeholder or calculated from first tick if possible
        plus_dm_hist_m.push_back(0);
        minus_dm_hist_m.push_back(0);
        dx_hist_m.push_back(0);
        return; 
    }

    if (datetime[idx].size() < 16) return; // Defensive: skip if not enough chars for time
    std::string tstr = datetime[idx].substr(11, 5);
    if (tstr < "09:15" || tstr > "15:30") return;
    // Indicators
    double ema_short = ema(close, short_ema, idx);
    double ema_long = ema(close, long_ema, idx);
    double rsi_val = rsi(close, rsi_period, idx);
    double vol_ma20 = rolling_mean(volume, 20, idx);
    // ATR
    double hl = high[idx] - low[idx];
    double hc = std::abs(high[idx] - close[idx - 1]);
    double lc = std::abs(low[idx] - close[idx - 1]);
    double tr = std::max({hl, hc, lc});
    tr_hist_m.push_back(tr);
    double atr = ema(tr_hist_m, atr_period, tr_hist_m.size() - 1);

    // ADX (simplified for demo)
    double up = high[idx] - high[idx - 1];
    double dn = low[idx - 1] - low[idx];
    double plus_dm = (up > dn && up > 0) ? up : 0;
    double minus_dm = (dn > up && dn > 0) ? dn : 0;
    
    plus_dm_hist_m.push_back(plus_dm);
    minus_dm_hist_m.push_back(minus_dm);

    // Ensure tr_hist_m is populated before calculating its EMA for tr_ema_hist
    // This check might be redundant if tr_hist_m is always populated before this point
    if (tr_hist_m.empty()) { 
        // This case should ideally not be reached if idx > 0 and tr_hist_m is handled correctly
        return; 
    }
    // We need to ensure that the index for tr_hist_m in ema calculation is valid
    double current_tr_ema = ema(tr_hist_m, adx_period, tr_hist_m.size() - 1);
    if (std::isnan(current_tr_ema)) { // Not enough data for TR EMA
        dx_hist_m.push_back(0); // Cannot calculate DX, push placeholder
        // adx will also be NAN or based on insufficient data
    } else {
        double plus_di = 100 * (ema(plus_dm_hist_m, adx_period, plus_dm_hist_m.size() -1) / (current_tr_ema == 0 ? 1e-10 : current_tr_ema));
        double minus_di = 100 * (ema(minus_dm_hist_m, adx_period, minus_dm_hist_m.size() -1) / (current_tr_ema == 0 ? 1e-10 : current_tr_ema));
        double dx_val = (plus_di + minus_di == 0) ? 0 : (100 * (std::abs(plus_di - minus_di) / (plus_di + minus_di)));
        dx_hist_m.push_back(dx_val);
    }

    double adx = ema(dx_hist_m, adx_period, dx_hist_m.size() - 1);

    // Trading logic
    if (position == 0) {
        // Entry condition
        if (idx > 0 && ema_short > ema_long && close[idx - 1] <= ema_long &&
            rsi_lb < rsi_val && rsi_val < rsi_ub && adx > adx_threshold && volume[idx] > vol_ma20) {
            double risk_amt = equity * risk_per_trade;
            double stop = close[idx] - atr;
            int qty = static_cast<int>(risk_amt / (close[idx] - stop));
            if (qty >= 1) {
                entry_price = close[idx] * (1 + slippage);
                tp_level = entry_price + 1.5 * atr;
                stop_level = entry_price - (entry_price - stop);
                original_stop_distance = entry_price - stop_level;
                position = qty;
                // Log entry
                std::ostringstream oss;
                oss << "ENTRY," << datetime[idx] << "," << entry_price << "," << qty << ",EQUITY," << equity;
                log_trade(oss.str());
            }
        }
    } else {
        bool is_heavy_loss = close[idx] < entry_price - 2 * atr;
        if (is_heavy_loss) stop_level = entry_price - 1.5 * original_stop_distance;
        double exit_price = NAN;
        if (low[idx] <= stop_level) exit_price = stop_level;
        else if (high[idx] >= tp_level) exit_price = tp_level;
        else if (!is_heavy_loss && close[idx] > entry_price) exit_price = close[idx] * (1 - slippage);
        if (!std::isnan(exit_price)) {
            double profit = (exit_price - entry_price) * position;
            double commission = 20 * 2 + (profit > 0 ? 0.01 * profit : 0);
            double net_pnl = profit - commission;
            equity += net_pnl;
            std::ostringstream oss;
            oss << "EXIT," << datetime[idx] << "," << exit_price << "," << position << ",PROFIT," << profit << ",COMMISSION," << commission << ",NET_PNL," << net_pnl << ",EQUITY," << equity;
            log_trade(oss.str());
            position = 0;
            entry_price = stop_level = tp_level = original_stop_distance = 0.0;
        }
    }
}

void SimpleSMABroadStrategy::on_fill(const FillEvent&) {}

void SimpleSMABroadStrategy::log_trade(const std::string& log_line) {
    trade_logs.push_back(log_line);
    if (trade_logs.size() >= 100) flush_logs();
}

void SimpleSMABroadStrategy::flush_logs() {
    std::ofstream ofs(log_path, std::ios::app);
    for (const auto& line : trade_logs) {
        ofs << line << std::endl;
    }
    trade_logs.clear();
}

} // namespace backtest
