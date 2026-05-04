// services/market_service.cpp
#include "market_service.h"
#include "common/trading_hours.h"
#include "events/event_system.h"
#include <iostream>
#include <algorithm>
#include <chrono>

MarketService& MarketService::Instance() {
    static MarketService ins;
    return ins;
}

void MarketService::initialize(EventSystem* event_sys) {
    event_system_ = event_sys;
}

void MarketService::addDataSource(std::unique_ptr<IDataSource> source) {
    sources_.push_back(std::move(source));
}

void MarketService::start() {
    if (running_.exchange(true)) return;
    worker_thread_ = std::thread(&MarketService::run_loop, this);
}

void MarketService::stop() {
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

bool MarketService::shouldFetchData() {
    // 简化逻辑：只要有任意一个关注的品种在交易时间，就获取数据
    // 你可以根据需要更复杂的逻辑，比如只获取特定品种的数据
    // 这里假设我们关注 "m" 和 "sr" 品种
    return TradingHours::isMarketOpen("m") || TradingHours::isMarketOpen("sr");
}

void MarketService::run_loop() {
    while (running_) {
        if (shouldFetchData()) {
            // 遍历所有数据源并获取数据
            for (auto& source : sources_) {
                // 这里需要知道数据源关注哪些品种，或者获取所有品种
                // 简化起见，假设每个数据源有预设的品种列表
                // 例如，DCEDataSource 关注 "m"
                if (source->getName() == "DCE" && TradingHours::isMarketOpen("m")) {
                    auto ticks = source->fetchQuotes("m"); // 传入关注的品种
                    cache_.update(ticks);
                    // 如果获取到了数据，发布更新事件
                    if (!ticks.empty() && event_system_) {
                        event_system_->publish(MarketUpdateEvent{});
                    }
                }
                if (source->getName() == "CZCE" && TradingHours::isMarketOpen("sr")) {
                    auto ticks = source->fetchQuotes("SR"); // 注意：API可能要求大写
                    cache_.update(ticks);
                    if (!ticks.empty() && event_system_) {
                        event_system_->publish(MarketUpdateEvent{});
                    }
                }
            }
        }
        // 睡眠一段时间，避免过于频繁的轮询
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void MarketService::refreshFromSource(const std::string& source_name, const std::string& identifier) {
    // ... (保持原有逻辑)
    auto it = std::find_if(sources_.begin(), sources_.end(),
        [&source_name](const std::unique_ptr<IDataSource>& src) {
            return src->getName() == source_name;
        });

    if (it != sources_.end()) {
        auto ticks = (*it)->fetchQuotes(identifier);
        cache_.update(ticks);
        if (!ticks.empty() && event_system_) {
            event_system_->publish(MarketUpdateEvent{});
        }
    }
    else {
        std::cerr << "DataSource '" << source_name << "' not found." << std::endl;
    }
}