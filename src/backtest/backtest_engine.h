// backtesting/backtest_engine.h
#pragma once
#include "../market_types.h"
#include "../events/event_system.h"
#include "../services/market_service.h"
#include "../data_sources/mysql_data_source.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>

class BacktestEngine {
public:
    BacktestEngine(EventSystem* event_system, MarketService* market_service);

    void addDataSource(std::unique_ptr<MysqlDataSource> source);
    void setBacktestPeriod(const std::string& start_date, const std::string& end_date);
    void run(); // 开始回测
    void pause();
    void resume();
    void stop();

private:
    EventSystem* event_system_;
    MarketService* market_service_;
    std::vector<std::unique_ptr<MysqlDataSource>> data_sources_;
    std::string start_date_;
    std::string end_date_;
    bool is_running_ = false;
    bool is_paused_ = false;

    void simulateTicks(); // 核心模拟循环
};