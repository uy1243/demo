// market_types.h
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <common/types.h>

// 数据源接口
class IDataSource {
public:
    virtual ~IDataSource() = default;
    // 获取指定品种或合约的行情
    virtual std::vector<TickData> fetchQuotes(const std::string& identifier) = 0;
    // 获取数据源名称
    virtual std::string getName() const = 0;
    // 启动数据源（如果需要持续更新）
    virtual void start() {}
    // 停止数据源
    virtual void stop() {}
    // 获取历史数据（用于回测）
    virtual std::multimap<std::string, TickData> fetchHistoricalData(
        const std::string& instrument,
        const std::string& start_time,
        const std::string& end_time) = 0;
};