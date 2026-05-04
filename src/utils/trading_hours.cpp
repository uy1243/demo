// common/trading_hours.cpp
#include "trading_hours.h"
#include <sstream>
#include <iomanip>
#include <iostream>

// 示例：设置豆粕(m)和白糖(SR)的交易时段 (CTP标准)
void TradingHours::setSchedule(const std::string& instrument_type, const std::vector<TradingSession>& sessions) {
    schedules_[instrument_type] = sessions;
}

// 获取品种代码前缀 (例如 "m2509" -> "m")
std::string getInstrumentPrefix(const std::string& instrument) {
    std::string prefix;
    for (char c : instrument) {
        if (std::isdigit(c)) break;
        prefix += std::tolower(c);
    }
    return prefix;
}

bool TradingHours::isMarketOpen(const std::string& instrument_full) {
    auto prefix = getInstrumentPrefix(instrument_full);
    auto it = schedules_.find(prefix);
    if (it == schedules_.end()) return false; // 默认不开盘

    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);

    for (const auto& session : it->second) {
        if (isWithinSession(now_tm, session)) {
            return true;
        }
    }
    return false;
}

bool TradingHours::isMarketOpen(const std::string& instrument_full, const std::string& time_str) {
    auto prefix = getInstrumentPrefix(instrument_full);
    auto it = schedules_.find(prefix);
    if (it == schedules_.end()) return false;

    std::tm check_tm = parseTimeString(time_str);

    for (const auto& session : it->second) {
        if (isWithinSession(check_tm, session)) {
            return true;
        }
    }
    return false;
}

bool TradingHours::isMarketOpening(const std::string& instrument_full, const std::string& time_str) {
    auto prefix = getInstrumentPrefix(instrument_full);
    auto it = schedules_.find(prefix);
    if (it == schedules_.end()) return false;

    std::tm check_tm = parseTimeString(time_str);

    for (const auto& session : it->second) {
        std::tm start_tm = parseTimeString(session.start_time);
        if (check_tm.tm_hour == start_tm.tm_hour && check_tm.tm_min == start_tm.tm_min) {
            return true; // 时间戳正好是某个session的开始时间
        }
    }
    return false;
}

bool TradingHours::isMarketClosing(const std::string& instrument_full, const std::string& time_str) {
    auto prefix = getInstrumentPrefix(instrument_full);
    auto it = schedules_.find(prefix);
    if (it == schedules_.end()) return false;

    std::tm check_tm = parseTimeString(time_str);

    for (const auto& session : it->second) {
        std::tm end_tm = parseTimeString(session.end_time);
        if (check_tm.tm_hour == end_tm.tm_hour && check_tm.tm_min == end_tm.tm_min) {
            return true; // 时间戳正好是某个session的结束时间
        }
    }
    return false;
}

std::tm TradingHours::parseTimeString(const std::string& time_str) {
    std::tm tm = {};
    std::istringstream ss(time_str);
    ss >> std::get_time(&tm, "%H:%M");
    return tm;
}

bool TradingHours::isWithinSession(const std::tm& check_time, const TradingSession& session) {
    std::tm start_tm = parseTimeString(session.start_time);
    std::tm end_tm = parseTimeString(session.end_time);

    // 简化比较，仅比较小时和分钟
    int check_minutes = check_time.tm_hour * 60 + check_time.tm_min;
    int start_minutes = start_tm.tm_hour * 60 + start_tm.tm_min;
    int end_minutes = end_tm.tm_hour * 60 + end_tm.tm_min;

    return (check_minutes >= start_minutes && check_minutes <= end_minutes);
}

// 初始化默认交易时段
namespace {
    void init_default_schedules() {
        TradingHours::setSchedule("m", { // 豆粕
            {"09:00", "10:15"},
            {"10:30", "11:30"},
            {"13:30", "15:00"}
            });
        TradingHours::setSchedule("sr", { // 白糖
            {"09:00", "10:15"},
            {"10:30", "11:30"},
            {"13:30", "15:00"}
            });
        TradingHours::setSchedule("if", { // 股指期货
            {"09:30", "11:30"},
            {"13:00", "15:00"}
            });
    }
    static auto init = init_default_schedules(); // 在库加载时初始化
}