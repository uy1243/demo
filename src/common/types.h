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

// 统一的行情数据结构
struct TickData {
    std::string instrument;    // 合约代码
    std::string source;        // 数据源标识 (e.g., "SINA", "DCE", "CZCE")
    std::string time;          // 时间戳
    double last = 0.0;         // 最新价
    double open = 0.0;         // 开盘价
    double high = 0.0;         // 最高价
    double low = 0.0;          // 最低价
    long long volume = 0;      // 成交量
    long long open_interest = 0; // 持仓量（新增保留）

    // --- 新增：市场深度数据 ---
    double bid1{ 0.0 };             // 买一价
    double bid2{ 0.0 };
    double bid3{ 0.0 };
    double bid4{ 0.0 };
    double bid5{ 0.0 };
    double ask1{ 0.0 };             // 卖一价
    double ask2{ 0.0 };
    double ask3{ 0.0 };
    double ask4{ 0.0 };
    double ask5{ 0.0 };

    long long bid_vol1{ 0 };        // 买一量
    long long bid_vol2{ 0 };
    long long bid_vol3{ 0 };
    long long bid_vol4{ 0 };
    long long bid_vol5{ 0 };
    long long ask_vol1{ 0 };        // 卖一量
    long long ask_vol2{ 0 };
    long long ask_vol3{ 0 };
    long long ask_vol4{ 0 };
    long long ask_vol5{ 0 };
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