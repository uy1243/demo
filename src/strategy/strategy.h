#pragma once
#include "datasource/market_data.h"
#include "state_machine/account.h"

class Strategy {
public:
    static Strategy& Instance();
    void runStrategy(const MarketCache& cache);  // 自动交易策略
private:
    Strategy() = default;
};