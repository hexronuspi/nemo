import backtest_engine as bt
import numpy as np
from collections import deque
from typing import Dict, Optional


class SMAStrategy:
    """Simple Moving Average Crossover Strategy"""
    
    def __init__(self, strategy_id: str, short_period: int = 12, long_period: int = 26):
        self.strategy_id = strategy_id
        self.short_period = short_period
        self.long_period = long_period
        
        # Price history storage
        self.price_histories: Dict[str, deque[float]] = {}
        
        # Position tracking
        self.positions: Dict[str, float] = {}
        
        # Signal state
        self.last_signals: Dict[str, Optional[str]] = {}
        
        bt.log_info(strategy_id, f"Initialized SMA Strategy (short={short_period}, long={long_period})")
    
    def initialize(self):
        """Called when strategy is initialized"""
        bt.log_info(self.strategy_id, "Strategy initialization complete")
    
    def on_market_data(self, instrument: str, timestamp: str, bid_price: float, 
                      ask_price: float, last_price: float):
        """Handle incoming market data"""
        
        # Initialize price history for new instrument
        if instrument not in self.price_histories:
            self.price_histories[instrument] = deque(maxlen=max(self.short_period, self.long_period))
            self.positions[instrument] = 0
            self.last_signals[instrument] = None
        
        # Add new price
        mid_price = (bid_price + ask_price) / 2.0
        self.price_histories[instrument].append(mid_price)
        
        # Check if we have enough data
        if len(self.price_histories[instrument]) < self.long_period:
            return
        
        # Calculate moving averages
        prices = list(self.price_histories[instrument])
        short_sma = np.mean(prices[-self.short_period:])
        long_sma = np.mean(prices[-self.long_period:])
        
        # Generate signals
        current_signal = None
        
        if short_sma > long_sma:
            # Bullish signal
            if self.last_signals[instrument] != 'BUY' and self.positions[instrument] <= 0:
                current_signal = 'BUY'
                
        elif short_sma < long_sma:
            # Bearish signal
            if self.last_signals[instrument] != 'SELL' and self.positions[instrument] >= 0:
                current_signal = 'SELL'
        
        # Emit signals if needed
        if current_signal == 'BUY':
            bt.signal_buy(self.strategy_id, instrument, strength=1.0)
            self.last_signals[instrument] = 'BUY'
            bt.log_debug(self.strategy_id, f"BUY signal for {instrument} - SMA_short={short_sma:.4f}, SMA_long={long_sma:.4f}")
            
        elif current_signal == 'SELL':
            bt.signal_sell(self.strategy_id, instrument, strength=1.0)
            self.last_signals[instrument] = 'SELL'
            bt.log_debug(self.strategy_id, f"SELL signal for {instrument} - SMA_short={short_sma:.4f}, SMA_long={long_sma:.4f}")
    
    def on_fill(self, instrument: str, side: str, quantity: float, price: float):
        """Handle trade fills"""
        
        if side.upper() == 'BUY':
            self.positions[instrument] = self.positions.get(instrument, 0) + quantity
        else:
            self.positions[instrument] = self.positions.get(instrument, 0) - quantity
        
        current_pnl = bt.get_strategy_pnl(self.strategy_id)
        
        bt.log_info(self.strategy_id, 
                   f"Fill: {side} {quantity} {instrument} @ {price:.4f}, "
                   f"Position: {self.positions[instrument]}, P&L: ${current_pnl:.2f}")
    
    def on_risk_event(self, risk_type: str, message: str):
        """Handle risk management events"""
        bt.log_warn(self.strategy_id, f"Risk event - {risk_type}: {message}")
    
    def get_positions(self):
        """Get current positions"""
        return self.positions.copy()
    
    def get_performance_summary(self):
        """Get performance summary"""
        total_pnl = bt.get_strategy_pnl(self.strategy_id)
        
        return {
            'strategy_id': self.strategy_id,
            'total_pnl': total_pnl,
            'positions': self.get_positions(),
            'parameters': {
                'short_period': self.short_period,
                'long_period': self.long_period
            }
        }


# Strategy factory function
def create_strategy(strategy_id: str, **kwargs) -> SMAStrategy:
    """Factory function to create SMA strategy instance"""
    short_period = kwargs.get('short_period', 12)
    long_period = kwargs.get('long_period', 26)
    
    return SMAStrategy(strategy_id, short_period, long_period)


# Example usage and testing
if __name__ == "__main__":
    # This would be called by the C++ engine
    print("SMA Strategy module loaded successfully")
    
    # Example strategy creation
    strategy = create_strategy("test_sma", short_period=10, long_period=20)
    print(f"Created strategy: {strategy.strategy_id}")
    print(f"Parameters: short={strategy.short_period}, long={strategy.long_period}")
