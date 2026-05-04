// common/types.h
#pragma once
#include <string>
#include <vector>

enum class OrderStatus {
    SUBMITTING,     // 提交中
    PENDING,        // 已报单
    PART_FILLED,    // 部分成交
    FILLED,         // 全部成交
    CANCELED,       // 已撤单
    REJECTED        // 已拒单
};

enum class Direction {
    LONG,  // 买
    SHORT  // 卖
};

struct TickData {
    std::string instrument;
    double last = 0.0;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    long long volume = 0;
    double bid1 = 0.0;
    double ask1 = 0.0;
    std::string time;
    std::string source; // 数据源标识
};

struct Order {
    std::string order_id;       // 订单ID (本地)
    std::string exchange_order_id; // 交易所订单ID
    std::string instrument;     // 合约
    Direction dir;              // 方向
    double price = 0.0;         // 价格
    int volume = 0;             // 总手数
    int filled = 0;             // 已成交
    OrderStatus status = OrderStatus::PENDING;
    std::string create_time;
};

struct Position {
    std::string instrument;
    Direction dir;
    int volume = 0;
    double avg_price = 0.0;  // 开仓均价
    float pnl = 0.0f;        // 浮动盈亏
};

struct AccountFund {
    double total_asset = 1000000.0;  // 总权益
    double available = 1000000.0;    // 可用资金
    double frozen = 0.0;             // 冻结资金
    double position_pnl = 0.0;       // 持仓盈亏
    double fee = 0.0;                // 手续费
};