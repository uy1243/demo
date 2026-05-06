// strategy.h
#pragma once
#include <string>
#include "common/types.h" // 包含 Direction 等
#include "strategy/event_system.h"

class Strategy {
public:
    static Strategy& Instance();
    void initialize(EventSystem* event_sys);

    // 当市场数据更新时被调用
    void on_market_update();
    struct StrategyState {
        double m2509_high_since_open = 0.0;      // 开盘以来的最高价
        double m2509_low_since_open = 99999.0;   // 开盘以来的最低价
        double sr2509_high_since_open = 0.0;     // 白糖开盘以来的最高价
        double sr2509_low_since_open = 99999.0;  // 白糖开盘以来的最低价

        // 订单簿快照（模拟）
        double best_bid = 0.0;
        double best_ask = 0.0;
        long long bid_volume = 0;
        long long ask_volume = 0;
    };
    StrategyState state_;
private:
    Strategy() = default;
    EventSystem* event_system_ = nullptr;

    // 发送交易信号
    void send_signal(const std::string& instrument, Direction dir, double price, int volume);
};