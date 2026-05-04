// market_cache.h
#pragma once
#include "market_types.h"
#include <shared_mutex>
#include <unordered_map>

using InstrumentKey = std::string; // 以合约代码作为主键

class MarketCache {
public:
    // 存储行情
    void update(const TickData& tick);
    // 批量存储行情
    void update(const std::vector<TickData>& ticks);
    // 获取单条行情
    bool get(const std::string& instrument, TickData& out_tick) const;
    // 获取所有行情
    std::unordered_map<InstrumentKey, TickData> getAll() const;

private:
    mutable std::shared_mutex mtx_; // 读写锁，提高并发性能
    std::unordered_map<InstrumentKey, TickData> cache_;
};