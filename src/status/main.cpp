#include <iostream>
#include <thread>
#include <chrono>
#include "market_data.h"
#include "analysis.h"
#include "account.h"
#include "account_view.h"
#include "strategy.h"
#include "kline.h"
#include "logger.h"
#include "monitor.h"
#include "settlement.h"

int main() {
    LOG_INFO("系统启动");

    MarketAnalyzer analyzer;
    auto& cache_mgr = MarketManager::Instance();
    auto& strategy = Strategy::Instance();
    auto& monitor = FundMonitor::Instance();
    auto& kline = KLineGenerator::Instance();

    while (true) {
        system("cls");

        // 行情
        GenerateRandomDceQuote();
        GenerateRandomCzceQuote();
        auto cache = cache_mgr.GetAllCache();

        // K线合成
        for (auto& p : cache) kline.tick(p.second);

        // 账户撮合
        Account::Instance().matchOrders(cache);
        Account::Instance().updatePnL(cache);

        // 策略自动下单
        strategy.runStrategy(cache);

        // 资金监控
        monitor.check();

        // 打印
        PrintMarketCache();
        analyzer.printAnalysis();
        AccountView::printAll();

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}