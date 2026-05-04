// backtesting/backtest_engine.cpp
#include "backtest_engine.h"
#include <iostream>
#include <thread>
#include <algorithm>
#include <queue> // 用于合并多个数据源的时间序列

BacktestEngine::BacktestEngine(EventSystem* event_system, MarketService* market_service)
    : event_system_(event_system), market_service_(market_service) {
}

void BacktestEngine::addDataSource(std::unique_ptr<MysqlDataSource> source) {
    data_sources_.push_back(std::move(source));
}

void BacktestEngine::setBacktestPeriod(const std::string& start_date, const std::string& end_date) {
    start_date_ = start_date; // 格式 "YYYY-MM-DD HH:MM:SS"
    end_date_ = end_date;
}

void BacktestEngine::run() {
    if (is_running_) return;
    is_running_ = true;
    is_paused_ = false;

    simulateTicks();
}

void BacktestEngine::pause() { is_paused_ = true; }
void BacktestEngine::resume() { is_paused_ = false; }
void BacktestEngine::stop() { is_running_ = false; }

void BacktestEngine::simulateTicks() {
    // 1. 从所有数据源获取历史数据，并合并成一个按时间排序的队列
    std::priority_queue<TickData, std::vector<TickData>,
        std::function<bool(const TickData&, const TickData&)>>
        tick_queue([](const TickData& a, const TickData& b) {
        return a.time > b.time; // 最小堆，时间早的在顶部
            });

    for (auto& source : data_sources_) {
        // 这里需要知道具体要回测哪些合约
        // 可以通过配置或扫描数据库获得
        std::vector<std::string> instruments_to_test = { "m2509", "SR2509" }; // 示例

        for (const auto& inst : instruments_to_test) {
            auto hist_data = source->fetchHistoricalData(inst, start_date_, end_date_);
            for (const auto& [time, tick] : hist_data) {
                tick_queue.push(tick);
            }
        }
    }

    // 2. 模拟时间流逝，逐个推送 tick
    while (is_running_ && !tick_queue.empty()) {
        if (is_paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        TickData current_tick = tick_queue.top();
        tick_queue.pop();

        // 3. 更新 MarketService 的缓存
        market_service_->getCache().update(current_tick);

        // 4. 发布市场更新事件，触发策略
        event_system_->publish(MarketUpdateEvent{});

        // 5. 模拟真实时间流逝（可选，用于调试慢速回测）
        // std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    }

    std::cout << "[BacktestEngine] Backtest completed." << std::endl;
    is_running_ = false;
}