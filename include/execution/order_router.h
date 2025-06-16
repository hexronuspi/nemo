#pragma once
#include <memory>
#include <string>
#include <vector>
#include "execution/order_book.h"
#include "utils/types.h"

namespace backtest {

class OrderRouter {
public:
    OrderRouter() = default;
    virtual ~OrderRouter() = default;
    virtual void route_order(const Order& order);
};

} // namespace backtest
