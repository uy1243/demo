#pragma once
#include <string>
#include <unordered_map>

// 全局风控配置
struct RiskConfig {
    // 账户级
    double max_daily_loss_pct = 0.05;    // 单日最大亏损 5%
    int max_total_positions = 20;        // 最大总持仓手数
    double max_single_pos_pct = 0.1;     // 单个品种最大仓位 10%

    // 品种默认止盈止损（跳点）
    std::unordered_map<std::string, int> stop_points = {
        {"p2509", 40}, {"rb2510", 30}, {"ma2509", 25}
    };
    std::unordered_map<std::string, int> profit_points = {
        {"p2509", 80}, {"rb2510", 60}, {"ma2509", 50}
    };

    // 移动止损
    int trailing_stop_step = 20;
    bool enable_trailing = true;

    // 波动率保护
    double max_price_fluctuation = 0.02;  // 2%波动则暂停开仓
    int max_order_per_minute = 20;       // 流控
};