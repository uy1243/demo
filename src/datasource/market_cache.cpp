// market_cache.cpp
#include "market_cache.h"

void MarketCache::update(const TickData& tick) {
    std::unique_lock lock(mtx_);
    cache_[tick.instrument] = tick;
}

void MarketCache::update(const std::vector<TickData>& ticks) {
    std::unique_lock lock(mtx_);
    for (const auto& tick : ticks) {
        cache_[tick.instrument] = tick;
    }
}

bool MarketCache::get(const std::string& instrument, TickData& out_tick) const {
    std::shared_lock lock(mtx_);
    auto it = cache_.find(instrument);
    if (it != cache_.end()) {
        out_tick = it->second;
        return true;
    }
    return false;
}

std::unordered_map<InstrumentKey, TickData> MarketCache::getAll() const {
    std::shared_lock lock(mtx_);
    return cache_;
}