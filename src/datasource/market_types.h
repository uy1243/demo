// market_types.h
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// 统一的行情数据结构
struct TickData {
    std::string instrument;    // 合约代码
    std::string source;        // 数据源标识 (e.g., "DCE", "CZCE", "SINA")
    std::string time;          // 时间戳
    double last = 0.0;         // 最新价
    double open = 0.0;         // 开盘价
    double high = 0.0;         // 最高价
    double low = 0.0;          // 最低价
    double bid1 = 0.0;         // 买一价
    double ask1 = 0.0;         // 卖一价
    long long volume = 0;      // 成交量
    long long open_interest = 0; // 持仓量
    // ... 可以根据需要添加更多字段
};

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
};