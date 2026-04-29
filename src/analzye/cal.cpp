#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>

// 一条行情数据
struct MarketData {
    std::string instrument_id;  // 合约
    double last_price;          // 最新价
    double bid_price1;          // 买一
    double ask_price1;          // 卖一
    int volume;                 // 成交量
    long long timestamp;        // 时间戳
};

// 单个合约的日内数据管理器
class InstrumentData {
public:
    std::vector<MarketData> data_list;  // 全部内存存储

    // 添加一条数据
    void add(const MarketData& data) {
        data_list.push_back(data);
    }

    // 获取数据条数
    int count() const {
        return data_list.size();
    }

    // 最新价均值
    double avg_price() const {
        if (data_list.empty()) return 0.0;
        double sum = 0.0;
        for (auto& d : data_list) sum += d.last_price;
        return sum / data_list.size();
    }

    // 成交量均值
    double avg_volume() const {
        if (data_list.empty()) return 0.0;
        long long sum = 0;
        for (auto& d : data_list) sum += d.volume;
        return (double)sum / data_list.size();
    }

    // 最高价
    double high() const {
        if (data_list.empty()) return 0.0;
        double m = data_list[0].last_price;
        for (auto& d : data_list) m = std::max(m, d.last_price);
        return m;
    }

    // 最低价
    double low() const {
        if (data_list.empty()) return 0.0;
        double m = data_list[0].last_price;
        for (auto& d : data_list) m = std::min(m, d.last_price);
        return m;
    }

    // 清空（换日用）
    void clear() {
        data_list.clear();
    }
};