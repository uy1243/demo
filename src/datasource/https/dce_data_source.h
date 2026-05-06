// data_sources/dce_data_source.h
#pragma once
#include "../market_types.h"
#include <string>
#include <vector>

class DceDataSource : public IDataSource {
public:
    std::string getName() const override { return "DCE"; }
    std::vector<TickData> fetchQuotes(const std::string& commodity) override;
    std::multimap<std::string, TickData> fetchHistoricalData(
        const std::string& instrument,
        const std::string& start_time,
        const std::string& end_time);
};