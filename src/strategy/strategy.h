// strategy.h
#pragma once
#include <string>
#include "common/types.h" // 包含 Direction 等
#include "events/event_system.h"

class Strategy {
public:
    static Strategy& Instance();
    void initialize(EventSystem* event_sys);

    // 当市场数据更新时被调用
    void on_market_update();

private:
    Strategy() = default;
    EventSystem* event_system_ = nullptr;

    // 发送交易信号
    void send_signal(const std::string& instrument, Direction dir, double price, int volume);
};