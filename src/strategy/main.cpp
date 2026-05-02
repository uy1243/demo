#include "analyze/market_analyzer.h"

int main() {
    // 创建分析类实例
    MarketAnalyzer analyzer;

    while (true) {
        // 1. 生成行情 → 写入缓存
        GenerateRandomDceQuote();
        GenerateRandomCzceQuote();

        // 2. 清屏 + 打印行情
        system("cls");
        PrintMarketCache();

        // 3. 分析模块：从缓存读取并统计
        analyzer.printAnalysis();

        // 每秒刷新
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}