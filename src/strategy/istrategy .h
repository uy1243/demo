#pragma once
#include "market_data.h"
#include "account.h"

class Strategy {
public:
    static Strategy& Instance();
    void runStrategy(const MarketCache& cache);  // 自动交易策略
private:
    Strategy() = default;
};