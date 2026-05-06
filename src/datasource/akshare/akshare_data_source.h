// data_sources/sina_data_source.h
#pragma once
#include "../market_types.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <mutex>

class AkshareDataSource : public IDataSource {
public:
    std::string getName() const override { return "AKSHARE"; }
    std::vector<TickData> fetchQuotes(const std::string& instrument) override;

    std::multimap<std::string, TickData> fetchHistoricalData(
        const std::string& instrument,
        const std::string& start_time,
        const std::string& end_time);


};