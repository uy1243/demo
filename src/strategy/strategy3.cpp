// strategy.cpp
#include "strategy.h"
#include "account.h"
#include "services/market_service.h"
#include "common/trading_hours.h" // 新增
#include "events/event_system.h"
#include <iostream>

void Strategy::initialize(EventSystem* event_sys) {
    event_system_ = event_sys;
    event_sys->subscribe<MarketUpdateEvent>("MARKET_UPDATE",
        [this](const MarketUpdateEvent& e) { this->on_market_update(); });
}

Strategy& Strategy::Instance() {
    static Strategy ins;
    return ins;
}

void Strategy::send_signal(const std::string& instrument, Direction dir, double price, int volume) {
    if (event_system_) {
        event_system_->publish(TradeSignalEvent(instrument, dir, price, volume));
        std::cout << "[Strategy] Signal sent: " <<
            (dir == Direction::LONG ? "BUY" : "SELL") <<
            " " << instrument << " @ " << price << " x " << volume << std::endl;
    }
}

void Strategy::on_market_update() {
    auto& market_service = MarketService::Instance();
    auto cache = market_service.getCache().getAll();
    auto& account = Account::Instance();
    auto positions = account.getAllPositions();

    for (const auto& [inst, tick] : cache) {
        // 1. 检查当前tick是否在该品种的交易时间内
        if (!TradingHours::isMarketOpen(inst, tick.time)) {
            continue; // 不在交易时间，跳过该tick
        }

        // 2. 检查是否是开盘时间点，可以执行特殊策略
        if (TradingHours::isMarketOpening(inst, tick.time)) {
            std::cout << "[Strategy] Opening time for " << inst << " @" << tick.time << std::endl;
            // 在开盘时间点可以执行特定逻辑，例如追涨杀跌
            // if (tick.last > previous_close) send_signal(inst, Direction::LONG, ...);
            // else if (tick.last < previous_close) send_signal(inst, Direction::SHORT, ...);
        }

        // 3. 基于价格的普通策略
        if (inst == "m2509") {
            // 假设我们只想在上午第一个交易时段 (9:00-10:15) 操作
            if (tick.time >= "09:00" && tick.time <= "10:15") {
                bool has_position = false;
                for (const auto& pos : positions) {
                    if (pos.instrument == "m2509") { has_position = true; break; }
                }
                if (!has_position && tick.last > 3460) {
                    send_signal("m2509", Direction::LONG, tick.last, 1);
                }
            }
        }
        if (inst == "SR2509") {
            // 白糖策略，同样限制时间段
            if (tick.time >= "13:30" && tick.time <= "15:00") {
                bool has_position = false;
                for (const auto& pos : positions) {
                    if (pos.instrument == "SR2509") { has_position = true; break; }
                }
                if (!has_position && tick.last < 6520) {
                    send_signal("SR2509", Direction::SHORT, tick.last, 1);
                }
            }
        }
    }
}