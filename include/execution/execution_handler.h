#pragma once
#include <memory>
#include <string>
#include "execution/order_book.h"
#include "utils/types.h"

namespace backtest {

class ExecutionHandler {
public:
    ExecutionHandler() = default;
    virtual ~ExecutionHandler() = default;
    virtual void execute_order(const Order& order);
};

} // namespace backtest
