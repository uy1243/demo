#pragma once
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include "../common/types.h"

// 一笔交易记录
struct TradeRecord {
    std::string instrument;
    Direction direction;       // 多/空
    double open_price;         // 开仓价
    double close_price;        // 平仓价
    int volume;                // 手数
    double profit;             // 盈亏
    double commission;         // 手续费
    std::string open_time;
    std::string close_time;
    bool is_win;               // 是否盈利
};

// 资金曲线点
struct EquityPoint {
    std::string time;
    double total_equity;
    double balance;
};

// 评测报告
struct EvaluationReport {
    // 基础
    double initial_capital = 100000.0;
    double final_capital = 0.0;
    double total_profit = 0.0;
    double total_return_pct = 0.0;
    double annual_return_pct = 0.0;

    // 风险
    double max_drawdown_pct = 0.0;
    double max_drawdown_value = 0.0;
    double sharpe_ratio = 0.0;

    // 交易统计
    int total_trades = 0;
    int win_trades = 0;
    int loss_trades = 0;
    double win_rate_pct = 0.0;
    double profit_factor = 0.0;
    double avg_win = 0.0;
    double avg_loss = 0.0;
    double profit_loss_ratio = 0.0;

    // 连续
    int max_consec_win = 0;
    int max_consec_loss = 0;

    // 时间
    std::string start_time;
    std::string end_time;
    int trade_days = 0;

    // 曲线 & 交易列表
    std::vector<TradeRecord> trades;
    std::vector<EquityPoint> equity_curve;
};

// 评测引擎
class EvaluationEngine {
public:
    static EvaluationEngine& Instance();

    // 添加一笔交易
    void addTrade(const TradeRecord& trade);

    // 添加资金曲线点
    void addEquityPoint(const EquityPoint& point);

    // 生成报告（核心！）
    EvaluationReport generateReport();

    // 清空数据（下次回测/实盘用）
    void reset();

private:
    EvaluationEngine() = default;
    std::vector<TradeRecord> trades_;
    std::vector<EquityPoint> equity_;
};