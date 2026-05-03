#include "market_analyzer.h"
#include <iostream>
#include <vector>
#include <cmath>

StatsResult MarketAnalyzer::computeStats() {
    // 1. 从全局缓存获取所有行情
    auto cache = MarketManager::Instance().GetAllCache();
    StatsResult res;
    res.count = cache.size();

    if (res.count == 0)
        return res;

    // 2. 提取最新价
    std::vector<double> prices;
    for (auto& pair : cache) {
        prices.push_back(pair.second.last);
    }

    // 3. 计算统计指标
    res.mean_price = calculateMean(prices);
    res.max_price = calculateMax(prices);
    res.min_price = calculateMin(prices);
    res.std_dev = calculateStd(prices, res.mean_price);

    return res;
}

void MarketAnalyzer::printAnalysis() {
    StatsResult res = computeStats();

    std::cout <<"\n==== 【分析模块】实时统计 ====\n";
    std::cout << "合约数量: " << res.count << "\n";
    printf("均价: %.2f\n", res.mean_price);
    printf("最高价: %.2f\n", res.max_price);
    printf("最低价: %.2f\n", res.min_price);
    printf("标准差: %.2f\n", res.std_dev);
}

double MarketAnalyzer::calculateMean(const std::vector<double>& values) {
    double sum = 0.0;
    for (double v : values) sum += v;
    return sum / values.size();
}

double MarketAnalyzer::calculateMax(const std::vector<double>& values) {
    double m = values[0];
    for (double v : values) if (v > m) m = v;
    return m;
}

double MarketAnalyzer::calculateMin(const std::vector<double>& values) {
    double m = values[0];
    for (double v : values) if (v < m) m = v;
    return m;
}

double MarketAnalyzer::calculateStd(const std::vector<double>& values, double mean) {
    double var = 0.0;
    for (double v : values) {
        var += pow(v - mean, 2);
    }
    var /= values.size();
    return sqrt(var);
}

