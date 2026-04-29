#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>

// 一条行情数据
struct MarketData {
    std::string instrument_id;
    double last_price;
    double bid_price1;
    double ask_price1;
    int volume;
    long long timestamp;
};

// 单个合约的日内数据管理器
class InstrumentData {
public:
    std::vector<MarketData> data_list;

    void add(const MarketData& data) {
        data_list.push_back(data);
    }

    int count() const {
        return (int)data_list.size();
    }

    // 最新价均值
    double avg_price() const {
        if (data_list.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& d : data_list) sum += d.last_price;
        return sum / data_list.size();
    }

    // 成交量均值
    double avg_volume() const {
        if (data_list.empty()) return 0.0;
        long long sum = 0;
        for (const auto& d : data_list) sum += d.volume;
        return (double)sum / data_list.size();
    }

    // 最高价（修复 Windows 宏冲突）
    double high() const {
        if (data_list.empty()) return 0.0;
        double m = data_list[0].last_price;
        for (const auto& d : data_list) {
            if (d.last_price > m)
                m = d.last_price;
        }
        return m;
    }

    // 最低价（修复 Windows 宏冲突）
    double low() const {
        if (data_list.empty()) return 0.0;
        double m = data_list[0].last_price;
        for (const auto& d : data_list) {
            if (d.last_price < m)
                m = d.last_price;
        }
        return m;
    }

    void clear() {
        data_list.clear();
    }
};