// data_sources/czce_data_source.h
#pragma once
#include "../market_types.h"
#include <string>
#include <vector>

class CzceDataSource : public IDataSource {
public:
    std::string getName() const override { return "CZCE"; }
    std::vector<TickData> fetchQuotes(const std::string& commodity) override;
};