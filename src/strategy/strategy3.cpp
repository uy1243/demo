// strategy.cpp
#include "strategy.h"
#include "../status/account.h"
#include "../datasource/market_service.h"
#include "utils/trading_hours.h" // 新增
#include "event_system.h"
#include <iostream>
#include <algorithm>

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
    }
}

// --- 辅助函数：检查指定合约和方向的持仓 ---
bool hasPosition(const std::vector<Position>& positions,
    const std::string& instrument,
    Direction dir) {
    return std::any_of(positions.begin(), positions.end(),
        [&instrument, dir](const Position& p) {
            return p.instrument == instrument && p.dir == dir;
        });
}

void Strategy::on_market_update() {
    auto& market_service = MarketService::Instance();
    auto cache = market_service.getCache().getAll();
    auto& account = Account::Instance();
    auto positions = account.getAllPositions();
    for (const auto& [inst, tick] : cache) {
        std::cout << "[Strategy] Processing tick for " << inst << " at " << tick.time << ": last=" << tick.last << std::endl;
        send_signal("a2509", Direction::LONG, tick.open, 1);
        return; // 先测试一个合约，后续再完善其他逻辑
        // 1. 检查当前tick是否在该品种的交易时间内
        if (!TradingHours::isMarketOpen(inst, tick.time)) {
		    std::cout << "[Strategy] " << inst << " is not in trading hours at " << tick.time << std::endl;
            continue; // 不在交易时间，跳过该tick
        }

        // 2. 检查是否是开盘时间点，可以执行特殊策略
        if (TradingHours::isMarketOpening(inst, tick.time)) {
            std::cout << "[Strategy] Opening time for " << inst << " @" << tick.time << std::endl;
            // 在开盘时间点重置日内策略相关的状态
            if (inst == "m2509") {
                state_.m2509_high_since_open = tick.last;
                state_.m2509_low_since_open = tick.last;
            }
            if (inst == "SR2509") {
                state_.sr2509_high_since_open = tick.last;
                state_.sr2509_low_since_open = tick.last;
            }
        }

        // 更新日内高/低点（仅在交易时间内更新）
        if (inst == "m2509") {
            state_.m2509_high_since_open = std::max(state_.m2509_high_since_open, tick.last);
            state_.m2509_low_since_open = std::min(state_.m2509_low_since_open, tick.last);
        }
        if (inst == "SR2509") {
            state_.sr2509_high_since_open = std::max(state_.sr2509_high_since_open, tick.last);
            state_.sr2509_low_since_open = std::min(state_.sr2509_low_since_open, tick.last);
        }

        // 3. 更新订单簿快照（如果tick数据包含深度信息）
        if (tick.bid1 > 0 && tick.ask1 > 0) { // 假设TickData结构中有bid1, ask1
            state_.best_bid = tick.bid1;
            state_.best_ask = tick.ask1;
            state_.bid_volume = tick.bid_vol1; // 假设有对应的量
            state_.ask_volume = tick.ask_vol1;
        }


        // --- 策略一：豆粕 M2509 - 简单突破策略 + 逆势加仓 ---
        if (inst == "m2509") {
            // 策略A: 上午突破策略 (9:00-10:15)
            if (tick.time >= "09:00:00" && tick.time <= "10:15:00") { // 注意时间格式，最好用HH:MM:SS
                bool has_long_pos = std::any_of(positions.begin(), positions.end(),
                    [&inst](const Position& p) {
                        return p.instrument == inst && p.dir == Direction::LONG;
                    });
                bool has_short_pos = std::any_of(positions.begin(), positions.end(),
                    [&inst](const Position& p) {
                        return p.instrument == inst && p.dir == Direction::SHORT;
                    });

                // 入场信号：突破开盘后最高价
                if (!has_long_pos && tick.last > state_.m2509_high_since_open) {
                    send_signal(inst, Direction::LONG, tick.last, 1);
                    std::cout << "[Strategy M2509] Long Breakout at " << tick.last << std::endl;
                }
                // 逆势信号：跌破开盘后最低价
                if (!has_short_pos && tick.last < state_.m2509_low_since_open) {
                    send_signal(inst, Direction::SHORT, tick.last, 1);
                    std::cout << "[Strategy M2509] Short Breakdown at " << tick.last << std::endl;
                }
            }

            // 策略B: 下午趋势跟踪策略 (13:30-14:30)
            if (tick.time >= "13:30:00" && tick.time <= "14:30:00") {
                bool has_long_pos = std::any_of(positions.begin(), positions.end(),
                    [&inst](const Position& p) {
                        return p.instrument == inst && p.dir == Direction::LONG;
                    });
                bool has_short_pos = std::any_of(positions.begin(), positions.end(),
                    [&inst](const Position& p) {
                        return p.instrument == inst && p.dir == Direction::SHORT;
                    });

                // 使用买卖盘口判断短期动量
                double spread = state_.best_ask - state_.best_bid;
                if (spread > 0 && state_.ask_volume > state_.bid_volume * 1.5) { // 卖盘压力大
                    if (!has_short_pos) {
                        send_signal(inst, Direction::SHORT, tick.last, 1);
                        std::cout << "[Strategy M2509] Short Momentum Signal @" << tick.last << std::endl;
                    }
                }
                else if (spread > 0 && state_.bid_volume > state_.ask_volume * 1.5) { // 买盘压力大
                    if (!has_long_pos) {
                        send_signal(inst, Direction::LONG, tick.last, 1);
                        std::cout << "[Strategy M2509] Long Momentum Signal @" << tick.last << std::endl;
                    }
                }
            }
        }

        // --- 策略二：白糖 SR2509 - 均值回归策略 + 波动率监控 ---
        if (inst == "SR2509") {

            // 策略A: 特定时段均值回归 (13:30 - 15:00)
            if (tick.time >= "13:30:00" && tick.time <= "15:00:00") {
                bool has_long_pos = std::any_of(positions.begin(), positions.end(),
                    [&inst](const Position& p) {
                        return p.instrument == inst && p.dir == Direction::LONG;
                    });
                bool has_short_pos = std::any_of(positions.begin(), positions.end(),
                    [&inst](const Position& p) {
                        return p.instrument == inst && p.dir == Direction::SHORT;
                    });

                // 计算一个简化的“波动率”（基于日内高低价差）
                double price_range = state_.sr2509_high_since_open - state_.sr2509_low_since_open;

                // 假设我们有一个预估的“中枢”价格，这里用开盘价近似
                static double sr2509_open_price = 0.0;
                if (TradingHours::isMarketOpening(inst, tick.time)) {
                    sr2509_open_price = tick.last; // 在开盘时记录
                }

                // 均值回归信号：当价格远离中枢且波动率较低时
                if (!has_long_pos && sr2509_open_price > 0 && price_range < 100) { // 波动率低
                    if (tick.last < sr2509_open_price - 50) { // 跌破中枢50点
                        send_signal(inst, Direction::LONG, tick.last, 1); // 抄底
                        std::cout << "[Strategy SR2509] Mean Reversion Long @" << tick.last << std::endl;
                    }
                }
                if (!has_short_pos && sr2509_open_price > 0 && price_range < 100) { // 波动率低
                    if (tick.last > sr2509_open_price + 50) { // 冲破中枢50点
                        send_signal(inst, Direction::SHORT, tick.last, 1); // 做空
                        std::cout << "[Strategy SR2509] Mean Reversion Short @" << tick.last << std::endl;
                    }
                }
            }

            // 策略B: 波动率突破策略 (全天，可调整)
            double price_range = state_.sr2509_high_since_open - state_.sr2509_low_since_open;
            if (price_range > 200) { // 当日内价格波动超过200点，认为波动率升高
                // 此时更适合趋势策略而非均值回归
                // 可以结合趋势指标（如MA）判断方向
                // 这里简单示例：如果当前价格接近当日高点，且未持有空单，则考虑做多
                if (!has_short_pos && std::abs(tick.last - state_.sr2509_high_since_open) < 10) {
                    send_signal(inst, Direction::LONG, tick.last, 1);
                    std::cout << "[Strategy SR2509] Volatility Breakout Long @" << tick.last << std::endl;
                }
            }
        }

        // --- 策略三：跨品种套利（示例） ---
        // 假设我们有豆粕和菜粕的tick
        // if (inst == "m2509" || inst == "rm2509") { // 需要同时接收两个合约的tick
        //     static double spread_last = 0.0;
        //     // 假设有一个函数计算价差
        //     // double current_spread = calculateSpread("m2509", "rm2509");
        //     // if (current_spread > upper_threshold) send_signal("m2509/rm2509", ...);
        //     // if (current_spread < lower_threshold) send_signal("m2509/rm2509", ...);
        // }

        // --- 策略四：风险控制 ---
        // 示例：对所有品种进行止损止盈
        for (const auto& pos : positions) {
            if (pos.instrument == inst) {
                double pnl = (pos.dir == Direction::LONG ? tick.last - pos.avg_price : pos.avg_price - tick.last) * pos.volume;
                // 绝对值止损：亏损超过100点
                if (std::abs(pnl) > 100 * pos.volume) {
                    Direction close_dir = (pos.dir == Direction::LONG ? Direction::SHORT : Direction::LONG);
                    send_signal(pos.instrument, close_dir, tick.last, pos.volume);
                    std::cout << "[Risk Control] Stop Loss/Profit for " << pos.instrument
                        << " PnL: " << pnl << std::endl;
                }
            }
        }

    }
}