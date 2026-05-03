#pragma once
#include "datasource/market_data.h"

// 统计结果
struct StatsResult {
    double mean_price = 0.0;
    double max_price = 0.0;
    double min_price = 0.0;
    double std_dev = 0.0;
    size_t count = 0;
};

// 分析模块类
class MarketAnalyzer {
public:
    // 从缓存计算统计
    StatsResult computeStats();

    // 输出分析结果
    void printAnalysis();

private:
    // 内部工具函数
    double calculateMean(const std::vector<double>& values);
    double calculateMax(const std::vector<double>& values);
    double calculateMin(const std::vector<double>& values);
    double calculateStd(const std::vector<double>& values, double mean);
};