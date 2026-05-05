#include "evaluation_report.h"
#include <algorithm>
#include <cmath>
#include <iostream>

EvaluationEngine& EvaluationEngine::Instance() {
    static EvaluationEngine ins;
    return ins;
}

void EvaluationEngine::addTrade(const TradeRecord& trade) {
    trades_.push_back(trade);
}

void EvaluationEngine::addEquityPoint(const EquityPoint& point) {
    equity_.push_back(point);
}

void EvaluationEngine::reset() {
    trades_.clear();
    equity_.clear();
}

EvaluationReport EvaluationEngine::generateReport() {
    EvaluationReport r;

    if (equity_.empty() || trades_.empty()) {
        std::cerr << "[评测] 无交易或无资金曲线\n";
        return r;
    }

    r.trades = trades_;
    r.equity_curve = equity_;
    r.initial_capital = equity_.front().total_equity;
    r.final_capital = equity_.back().total_equity;
    r.total_profit = r.final_capital - r.initial_capital;
    r.total_return_pct = r.total_profit / r.initial_capital * 100.0;
    r.start_time = equity_.front().time;
    r.end_time = equity_.back().time;
    r.total_trades = (int)trades_.size();

    // 胜率 & 盈亏
    double gross_profit = 0, gross_loss = 0;
    int consec_win = 0, consec_loss = 0;
    for (auto& t : trades_) {
        if (t.is_win) {
            r.win_trades++;
            gross_profit += t.profit;
            consec_win++;
            consec_loss = 0;
            r.max_consec_win = std::max(r.max_consec_win, consec_win);
        }
        else {
            gross_loss += fabs(t.profit);
            consec_loss++;
            consec_win = 0;
            r.max_consec_loss = std::max(r.max_consec_loss, consec_loss);
        }
    }

    r.win_rate_pct = r.total_trades > 0 ? (double)r.win_trades / r.total_trades * 100.0 : 0.0;
    r.profit_factor = gross_loss > 0 ? gross_profit / gross_loss : 1e9;
    r.avg_win = r.win_trades > 0 ? gross_profit / r.win_trades : 0;
    r.avg_loss = r.loss_trades > 0 ? gross_loss / r.loss_trades : 0;
    r.profit_loss_ratio = r.avg_loss > 0 ? r.avg_win / r.avg_loss : 1e9;

    // 最大回撤
    double peak = equity_[0].total_equity;
    double max_dd_val = 0;
    double max_dd_pct = 0;
    for (auto& e : equity_) {
        peak = std::max(peak, e.total_equity);
        double dd_val = peak - e.total_equity;
        double dd_pct = dd_val / peak * 100.0;
        if (dd_val > max_dd_val) {
            max_dd_val = dd_val;
            max_dd_pct = dd_pct;
        }
    }
    r.max_drawdown_value = max_dd_val;
    r.max_drawdown_pct = max_dd_pct;

    return r;
}