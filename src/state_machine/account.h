#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "market_data.h"

// ==========================
// 1. 订单状态枚举
// ==========================
enum class OrderStatus {
    SUBMITTING,     // 提交中
    PENDING,        // 已报单
    PART_FILLED,    // 部分成交
    FILLED,         // 全部成交
    CANCELED,       // 已撤单
    REJECTED        // 已拒单
};

// 买卖方向
enum class Direction {
    LONG,  // 买
    SHORT  // 卖
};

// ==========================
// 2. 订单结构体
// ==========================
struct Order {
    std::string order_id;       // 订单ID
    std::string instrument;     // 合约
    Direction dir;             // 方向
    double price = 0.0;         // 价格
    int volume = 0;             // 总手数
    int filled = 0;             // 已成交
    OrderStatus status = OrderStatus::PENDING;
    std::string create_time;
};

// ==========================
// 3. 持仓结构体
// ==========================
struct Position {
    std::string instrument;
    Direction dir;
    int volume = 0;
    double avg_price = 0.0;  // 开仓均价
    float pnl = 0.0f;        // 浮动盈亏
};

// ==========================
// 4. 资金结构体
// ==========================
struct AccountFund {
    double total_asset = 1000000.0;  // 总权益
    double available = 1000000.0;    // 可用资金
    double frozen = 0.0;             // 冻结资金
    double position_pnl = 0.0;       // 持仓盈亏
    double fee = 0.0;                // 手续费
};

// ==========================
// 账户管理类（单例）
// ==========================
class Account {
public:
    static Account& Instance();

    // 下单接口
    std::string buy(const std::string& inst, double price, int vol);
    std::string sell(const std::string& inst, double price, int vol);

    // 撤单
    void cancelOrder(const std::string& order_id);

    // 模拟撮合（每秒自动撮合）
    void matchOrders(const MarketCache& cache);

    // 计算持仓盈亏
    void updatePnL(const MarketCache& cache);

    // 查询接口
    std::vector<Order> getAllOrders();
    std::vector<Position> getAllPositions();
    AccountFund getFundInfo();

private:
    Account() = default;
    std::mutex mtx_;

    // 订单、持仓、资金
    std::unordered_map<std::string, Order> orders_;
    std::unordered_map<std::string, Position> positions_;
    AccountFund fund_;

    // 内部工具
    std::string genOrderId();
    Position* getPosition(const std::string& inst, Direction dir);
    void updateFund(double fee, double frozen_delta);
};