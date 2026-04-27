#include "RiskManager.h"
#include <time.h>

void RiskManager::log_risk(const std::string& instr, const std::string& typ, const std::string& lvl, const std::string& msg) {
    if (!db) return;
    char sql[1024];
    sprintf(sql, "INSERT INTO risk_control_log(instrument,risk_type,level,message) VALUES('%s','%s','%s','%s')",
            instr.c_str(), typ.c_str(), lvl.c_str(), msg.c_str());
    db->execute(sql);
}

bool RiskManager::check_open(const std::string& instr, int vol, double capital) {
    std::lock_guard<std::mutex> lock(mtx);

    // 1. 仓位限制
    if (pos_map.size() >= config.max_total_positions) {
        log_risk(instr, "OVER_POS", "WARNING", "总持仓超限");
        return false;
    }

    // 2. 单品种限制
    double single_pct = (vol * 10000.0) / capital;
    if (single_pct > config.max_single_pos_pct) {
        log_risk(instr, "SINGLE_OVER", "WARNING", "单品种仓位超限");
        return false;
    }

    // 3. 波动率、流控等可扩展
    return true;
}

void RiskManager::add_position(const std::string& instr, char dir, int vol, double price) {
    std::lock_guard<std::mutex> lock(mtx);
    PositionRisk pr;
    pr.instrument = instr;
    pr.direction = dir;
    pr.volume = vol;
    pr.open_price = price;
    pr.high_water = price;
    pr.low_water = price;
    pos_map[instr + "_" + dir] = pr;
}

void RiskManager::check_all_positions(const std::map<std::string, double>& price_map) {
    std::lock_guard<std::mutex> lock(mtx);

    for (auto& [key, pos] : pos_map) {
        auto it = price_map.find(pos.instrument);
        if (it == price_map.end()) continue;
        double now = it->second;

        int stop = config.stop_points[pos.instrument];
        int profit = config.profit_points[pos.instrument];

        // 多单
        if (pos.direction == '0') {
            // 移动止损更新高水位
            if (now > pos.high_water) pos.high_water = now;

            // 止损
            if (pos.open_price - now > stop) {
                log_risk(pos.instrument, "STOP_LOSS", "FORCE_CLOSE", "多单止损");
                // 调用平仓接口
                return;
            }
            // 止盈
            if (now - pos.open_price > profit) {
                log_risk(pos.instrument, "TAKE_PROFIT", "FORCE_CLOSE", "多单止盈");
                return;
            }
            // 移动止损
            if (config.enable_trailing && pos.high_water - now > config.trailing_stop_step) {
                log_risk(pos.instrument, "TRAILING_STOP", "FORCE_CLOSE", "移动止损触发");
                return;
            }
        }
        // 空单
        else if (pos.direction == '1') {
            if (now < pos.low_water) pos.low_water = now;

            if (now - pos.open_price > stop) {
                log_risk(pos.instrument, "STOP_LOSS", "FORCE_CLOSE", "空单止损");
                return;
            }
            if (pos.open_price - now > profit) {
                log_risk(pos.instrument, "TAKE_PROFIT", "FORCE_CLOSE", "空单止盈");
                return;
            }
            if (config.enable_trailing && now - pos.low_water > config.trailing_stop_step) {
                log_risk(pos.instrument, "TRAILING_STOP", "FORCE_CLOSE", "空单移动止损");
                return;
            }
        }
    }
}

bool RiskManager::check_daily_loss(double today_pnl, double capital) {
    if (capital <= 0) return false;
    double pct = -today_pnl / capital;
    if (pct > config.max_daily_loss_pct) {
        log_risk("GLOBAL", "DAILY_LOSS", "FORCE_CLOSE", "单日亏损超限，全仓强平");
        return false;
    }
    return true;
}

bool RiskManager::check_order_rate() {
    static int cnt = 0;
    static time_t last = time(nullptr);
    time_t now = time(nullptr);

    if (now - last > 60) {
        cnt = 0;
        last = now;
    }
    if (++cnt > config.max_order_per_minute) {
        log_risk("GLOBAL", "ORDER_RATE", "WARNING", "下单频率超限");
        return false;
    }
    return true;
}