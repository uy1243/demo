// data_sources/mock_data_source.h
#pragma once
#include "common/types.h"
#include <random>
#include <chrono>

class MockDataSource : public IDataSource {
public:
    MockDataSource() : gen_(rd_()) {
        // 初始化模拟数据
        initializeMockData();
    }

    std::string getName() const override { return "MOCK"; }

    std::vector<TickData> fetchQuotes(const std::string& instrument) override {
        std::vector<TickData> result;

        if (instrument == "a2309") {
            result = generateMockTicks("a2309", 3450.0, 15.0);
        }
        else if (instrument == "SR2509") {
            result = generateMockTicks("SR2509", 6510.0, 20.0);
        }

        return result;
    }

    std::multimap<std::string, TickData> fetchHistoricalData(
        const std::string& instrument,
        const std::string& start_time,
        const std::string& end_time) override {

        std::multimap<std::string, TickData> sorted_ticks;

        // 生成5天的模拟数据，每分钟一个tick
        double base_price = (instrument == "m2509") ? 3450.0 : 6510.0;
        double volatility = (instrument == "m2509") ? 15.0 : 20.0;

        // 生成交易时间段（上午和下午）
        std::vector<std::string> morning_sessions;
        std::vector<std::string> afternoon_sessions;

        // 上午 9:00 - 11:30
        for (int hour = 9; hour <= 11; ++hour) {
            int max_minute = (hour == 11) ? 30 : 59;
            for (int minute = 0; minute <= max_minute; ++minute) {
                std::string time_str = (hour < 10 ? "0" : "") + std::to_string(hour) + ":" +
                    (minute < 10 ? "0" : "") + std::to_string(minute);
                morning_sessions.push_back(time_str);
            }
        }

        // 下午 13:30 - 15:00
        for (int hour = 13; hour <= 15; ++hour) {
            int start_minute = (hour == 13) ? 30 : 0;
            int max_minute = (hour == 15) ? 0 : 59;
            for (int minute = start_minute; minute <= max_minute; ++minute) {
                std::string time_str = std::to_string(hour) + ":" +
                    (minute < 10 ? "0" : "") + std::to_string(minute);
                afternoon_sessions.push_back(time_str);
            }
        }

        // 合并所有交易时段
        std::vector<std::string> all_sessions;
        all_sessions.insert(all_sessions.end(), morning_sessions.begin(), morning_sessions.end());
        all_sessions.insert(all_sessions.end(), afternoon_sessions.begin(), afternoon_sessions.end());

        // 生成5天的数据
        for (int day = 0; day < 5; ++day) {
            double day_base = base_price + std::normal_distribution<double>(0, volatility * 2)(gen_);
            double trend = std::normal_distribution<double>(0, volatility / 4)(gen_);

            for (size_t i = 0; i < all_sessions.size(); ++i) {
                TickData tick;
                tick.instrument = instrument;
                tick.source = getName();

                // 生成日期
                int month = 5;
                int day_of_month = 20 + day;
                if (day_of_month > 31) {
                    month = 6;
                    day_of_month -= 31;
                }

                // 修复：使用 std::ostringstream 进行字符串拼接
                std::ostringstream time_ss;
                time_ss << "2024-"
                    << (month < 10 ? "0" : "") << month << "-"
                    << (day_of_month < 10 ? "0" : "") << day_of_month << " "
                    << all_sessions[i] << ":00";
                tick.time = time_ss.str();

                // 生成随机价格（带趋势）
                double price_change = std::normal_distribution<double>(trend, volatility / 3)(gen_);
                tick.last = day_base + price_change + (i * trend / all_sessions.size());
                tick.open = tick.last - std::uniform_real_distribution<double>(-5, 5)(gen_);
                tick.high = tick.last + std::abs(std::normal_distribution<double>(0, volatility / 2)(gen_));
                tick.low = tick.last - std::abs(std::normal_distribution<double>(0, volatility / 2)(gen_));
                tick.volume = std::uniform_int_distribution<long long>(1000, 50000)(gen_);
                tick.bid1 = tick.last - 0.5;
                tick.ask1 = tick.last + 0.5;

                sorted_ticks.emplace(tick.time, tick);
            }
        }

        std::cout << "[MockDataSource] 生成 " << instrument
            << " 模拟数据: " << sorted_ticks.size() << " 条" << std::endl;

        return sorted_ticks;
    }


private:
    std::random_device rd_;
    std::mt19937 gen_;
    std::vector<TickData> mock_data_;

    void initializeMockData() {
        // 初始化一些模拟数据点
        mock_data_ = generateMockTicks("m2509", 3450.0, 15.0);
        auto sr_data = generateMockTicks("SR2509", 6510.0, 20.0);
        mock_data_.insert(mock_data_.end(), sr_data.begin(), sr_data.end());
    }

    std::vector<TickData> generateMockTicks(const std::string& instrument,
        double base_price,
        double volatility) {
        std::vector<TickData> ticks;

        // 生成一天的模拟数据
        for (int i = 0; i < 240; ++i) { // 4小时 * 60分钟
            TickData tick;
            tick.instrument = instrument;
            tick.source = getName();

            // 生成时间
            int hour = 9 + i / 60;
            int minute = i % 60;
            if (hour >= 12) hour += 1; // 跳过午休
            tick.time = "2024-05-20 " +
                std::to_string(hour) + ":" +
                std::to_string(minute) + ":00";

            // 生成价格
            double price_change = std::normal_distribution<double>(0, volatility / 3)(gen_);
            tick.last = base_price + price_change;
            tick.open = tick.last - std::uniform_real_distribution<double>(-5, 5)(gen_);
            tick.high = tick.last + std::abs(std::normal_distribution<double>(0, volatility / 2)(gen_));
            tick.low = tick.last - std::abs(std::normal_distribution<double>(0, volatility / 2)(gen_));
            tick.volume = std::uniform_int_distribution<long long>(1000, 50000)(gen_);
            tick.bid1 = tick.last - 0.5;
            tick.ask1 = tick.last + 0.5;

            ticks.push_back(tick);
        }

        return ticks;
    }
};