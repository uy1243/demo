// services/market_service.h
#pragma once
#include "market_cache.h"
#include "market_types.h"
#include "events/event_system.h"
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>

class MarketService {
public:
    static MarketService& Instance(); // 单例模式
    void initialize(EventSystem* event_sys);

    void addDataSource(std::unique_ptr<IDataSource> source);
    void start();
    void stop();
    void refreshFromSource(const std::string& source_name, const std::string& identifier);
    MarketCache& getCache() { return cache_; }

private:
    MarketService() = default;
    std::vector<std::unique_ptr<IDataSource>> sources_;
    MarketCache cache_;
    EventSystem* event_system_ = nullptr;
    std::atomic<bool> running_{ false };
    std::thread worker_thread_;

    void run_loop();
    bool shouldFetchData(); // 新增：判断是否应该获取数据
};