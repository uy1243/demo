// backtesting/backtest_engine.h
#pragma once
#include "common/types.h"
#include "strategy/event_system.h"
#include "datasource/market_service.h"
#include "datasource/mysql_db/mysql_data_source.h"
#include "status/account.h"
#include "strategy/strategy.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <queue>
#include <functional>

// 回测结果结构体
struct BacktestResult {
    double initial_capital = 1000000.0;  // 初始资金
    double final_capital = 0.0;          // 最终资金
    double total_return = 0.0;           // 总收益率
    double annual_return = 0.0;          // 年化收益率
    double max_drawdown = 0.0;           // 最大回撤
    double sharpe_ratio = 0.0;           // 夏普比率
    double total_fee = 0.0;              // 总手续费
    double total_pnl = 0.0;              // 总盈亏
    int total_trades = 0;                // 总交易次数
    double win_rate = 0.0;               // 胜率
    std::vector<double> equity_curve;    // 权益曲线
    std::vector<std::string> trade_dates; // 交易日期
    std::vector<double> drawdown_curve;  // 回撤曲线
};

class BacktestEngine {
public:
    BacktestEngine(EventSystem* event_system, MarketService* market_service);
    ~BacktestEngine();
    void stop();

    // 添加 MySQL 数据源
    void addDataSource(std::unique_ptr<IDataSource> source);

    // 设置回测参数
    void setBacktestPeriod(const std::string& start_date, const std::string& end_date);
    void setInitialCapital(double capital);
    void setCommissionRate(double rate);  // 手续费率
    void setSlippage(double slippage);    // 滑点

    // 设置回测的合约列表
    void setInstruments(const std::vector<std::string>& instruments);

    // 运行回测
    void run();

    // 获取回测结果
    BacktestResult getResult() const;

    // 打印回测报告
    void printReport() const;

private:
    EventSystem* event_system_;
    MarketService* market_service_;
    std::vector<std::unique_ptr<IDataSource>> data_sources_;
    std::vector<std::string> instruments_;

    // 回测参数
    std::string start_date_;
    std::string end_date_;
    double initial_capital_ = 1000000.0;
    double commission_rate_ = 0.0001;  // 万分之一
    double slippage_ = 0.0;            // 滑点

    // 回测状态
    std::atomic<bool> running_{ false };
    std::atomic<bool> paused_{ false };
    BacktestResult result_;


    // 辅助函数
    void simulateTicks();
    void calculateMetrics();
    double calculateSharpeRatio(const std::vector<double>& returns);
    double calculateMaxDrawdown(const std::vector<double>& equity);
    double calculateWinRate(const std::vector<double>& trade_pnls);

    // 模拟账户（用于回测，不连接真实交易接口）
    class SimulatedTrader : public ITrader {
    public:
        SimulatedTrader(BacktestEngine* engine, EventSystem* event_system);

        void initialize(EventSystem* event_system) override;

        OrderResponse placeOrder(const OrderRequest& req) override;
        CancelResponse cancelOrder(const CancelRequest& req) override;
        void start() override {}
        void stop() override {}
        std::string getName() const override { return "SIMULATED"; }

    private:
        BacktestEngine* engine_;
        EventSystem* event_system_;
        double commission_rate_;
        double slippage_;
    };

    friend class SimulatedTrader;
};