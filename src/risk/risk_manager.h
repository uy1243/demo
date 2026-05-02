#pragma once
#include <string>
#include <mutex>
#include <map>
#include "OrderDef.h"
#include "MySQLDB.h"

struct PositionRisk {
    std::string instrument;
    char direction;
    int volume;
    double open_price;
    double high_water;    // 移动止损最高价
    double low_water;     // 移动止损最低价
};

class RiskManager {
public:
    static RiskManager& instance() {
        static RiskManager ins;
        return ins;
    }

    void set_db(MySQLDB* db) { this->db = db; }
    void init(const RiskConfig& cfg) { config = cfg; }

    // 检查是否允许开仓
    bool check_open(const std::string& instr, int vol, double capital);

    // 新增持仓
    void add_position(const std::string& instr, char dir, int vol, double price);

    // 检查持仓是否触发止盈止损
    void check_all_positions(const std::map<std::string, double>& price_map);

    // 检查单日亏损
    bool check_daily_loss(double today_pnl, double capital);

    // 流控
    bool check_order_rate();

private:
    RiskManager() = default;
    RiskConfig config;
    std::map<std::string, PositionRisk> pos_map;
    std::mutex mtx;
    MySQLDB* db = nullptr;

    void log_risk(const std::string& instr, const std::string& typ, const std::string& lvl, const std::string& msg);
};