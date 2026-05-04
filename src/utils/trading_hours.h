// common/trading_hours.h
#pragma once
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <ctime>

struct TradingSession {
    std::string start_time; // 格式 "HH:MM"
    std::string end_time;   // 格式 "HH:MM"
};

class TradingHours {
public:
    // 设置特定品种的交易时段
    static void setSchedule(const std::string& instrument_type, const std::vector<TradingSession>& sessions);

    // 检查当前时间是否是该品种的交易时间
    static bool isMarketOpen(const std::string& instrument_type);

    // 检查给定时间戳是否是该品种的交易时间
    static bool isMarketOpen(const std::string& instrument_type, const std::string& time_str);

    // 检查是否是开盘时间点 (例如，上午9:00, 下午13:30)
    static bool isMarketOpening(const std::string& instrument_type, const std::string& time_str);

    // 检查是否是收盘时间点 (例如，上午10:15, 下午15:00)
    static bool isMarketClosing(const std::string& instrument_type, const std::string& time_str);

private:
    static std::set<std::string> parseTimeRange(const std::string& start, const std::string& end);
    static std::tm parseTimeString(const std::string& time_str);
    static bool isWithinSession(const std::tm& check_time, const TradingSession& session);

    static inline std::unordered_map<std::string, std::vector<TradingSession>> schedules_;
};