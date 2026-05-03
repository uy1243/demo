#include "strategy.h"
#include "logger.h"

Strategy& Strategy::Instance() {
    static Strategy ins;
    return ins;
}

void Strategy::runStrategy(const MarketCache& cache) {
    auto& acc = Account::Instance();

    // 简单趋势策略：价格 > 均价 自动买多；< 均价 自动卖空
    for (auto& [inst, q] : cache) {
        if (inst == "m2509") {
            if (q.last > 3460 && acc.getAllPositions().size() < 2) {
                std::string id = acc.buy(inst, q.last, 1);
                LOG_INFO("策略下单: %s 买 @ %.2f", inst.c_str(), q.last);
            }
        }
        if (inst == "SR2509") {
            if (q.last < 6520 && acc.getAllPositions().size() < 2) {
                std::string id = acc.sell(inst, q.last, 1);
                LOG_INFO("策略下单: %s 卖 @ %.2f", inst.c_str(), q.last);
            }
        }
    }
}