#include "datasource/ctp/md_spi.h"
#include "datasource/ctp/trader_spi.h"
#include "utils/logger.h"
#include "datasource/market_data.h"
#include "state_machine/account.h"
#include "strategy/strategy.h"
#include "gui/gui_operator/win32_auto.hpp"
#include "strategy/market_analyzer.h"
#include "datasource/mysql_db/db.hpp"
#include <windows.h>
#include "events/event_system.h"
#include "services/market_service.h"
#include "state_machine/account.h"
#include "trading/strategy.h"
#include "data_sources/dce_data_source.h"
#include "data_sources/czce_data_source.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "events/event_system.h"
#include "services/market_service.h"
#include "state_machine/account.h"
#include "trading/strategy.h"
#include "backtesting/backtest_engine.h"
#include "data_sources/mysql_data_source.h"
#include <iostream>
#include "events/event_system.h"
#include "services/market_service.h"
#include "state_machine/account.h"
#include "trading/strategy.h"
#include "trading/ctp_trader.h"
#include "trading/your_api_trader.h" // 如果需要
#include "backtesting/backtest_engine.h" // 如果需要
#include "data_sources/mysql_data_source.h" // 如果需要
#include <iostream>
// 👇 加上这一行，解决中文乱码
#pragma comment(linker, "/ENTRY:mainCRTStartup")

int main2(int argc, char* argv[]) {
    EventSystem event_system;

    // --- 选择运行模式 ---
    if (argc > 1 && std::string(argv[1]) == "backtest") {
        std::cout << "Starting Backtest..." << std::endl;
        // ... (回测逻辑，类似之前的代码) ...
        // 1. 创建 BacktestEngine
        // 2. 添加 MysqlDataSource
        // 3. 设置周期
        // 4. 启动 (此时 Account 使用模拟 trader)
        // 5. 输出结果
        return 0;
    }

    std::cout << "Starting Live Trading..." << std::endl;

    // --- 初始化真实交易接口 ---
    // 选择使用哪个 API
    std::unique_ptr<ITrader> primary_trader;
    std::unique_ptr<ITrader> backup_trader; // 可选

    // 例如，使用 CTP
    primary_trader = std::make_unique<CtpTrader>(
        "tcp://180.168.146.187:10000", // CTP前置地址
        "9999", // BrokerID
        "111111", // InvestorID
        "111111"  // Password
    );
    // primary_trader = std::make_unique<YourApiTrader>("path/to/config.json"); // 或者使用你的API

    // --- 初始化账户，注入真实交易接口 ---
    auto& account = Account::Instance();
    account.initialize(&event_system, primary_trader.get()); // 注入主交易接口

    // --- 初始化策略 ---
    auto& strategy = Strategy::Instance();
    strategy.initialize(&event_system);

    // --- 初始化市场服务 ---
    auto& market_service = MarketService::Instance();
    market_service.initialize(&event_system);

    // --- 订阅事件 ---
    event_system.subscribe<OrderUpdateEvent>("ORDER_UPDATE",
        [](const OrderUpdateEvent& e) { Account::Instance().on_order_update(e); });
    event_system.subscribe<TradeSignalEvent>("TRADE_SIGNAL",
        [](const TradeSignalEvent& e) {
            Account::Instance().execute_order(e.instrument, e.direction, e.price, e.volume);
        });

    // --- 启动 ---
    market_service.start();
    primary_trader->initialize(&event_system); // 传递 event_system 给 trader
    primary_trader->start();

    // --- 主循环 ---
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        event_system.publish(MarketUpdateEvent{});
    }

    // --- 清理 ---
    market_service.stop();
    primary_trader->stop();

    return 0;
}

int bc(int argc, char* argv[]) {
    EventSystem event_system;

    // --- 初始化 ---
    auto& account = Account::Instance();
    account.initialize(&event_system);

    auto& strategy = Strategy::Instance();
    strategy.initialize(&event_system);

    auto& market_service = MarketService::Instance();
    market_service.initialize(&event_system);

    // --- 设置事件订阅 ---
    event_system.subscribe<OrderUpdateEvent>("ORDER_UPDATE",
        [](const OrderUpdateEvent& e) { Account::Instance().on_order_update(e); });
    event_system.subscribe<TradeSignalEvent>("TRADE_SIGNAL",
        [](const TradeSignalEvent& e) {
            Account::Instance().execute_order(e.instrument, e.direction, e.price, e.volume);
        });

    // --- 决定运行模式 ---
    if (argc > 1 && std::string(argv[1]) == "backtest") {
        // --- 回测模式 ---
        std::cout << "Starting Backtest..." << std::endl;

        // 1. 创建回测引擎
        BacktestEngine backtest_engine(&event_system, &market_service);

        // 2. 添加 MySQL 数据源
        auto mysql_source = std::make_unique<MysqlDataSource>(
            "127.0.0.1", 33060, "root", "Yu646010", "black"
        );
        backtest_engine.addDataSource(std::move(mysql_source));

        // 3. 设置回测周期
        backtest_engine.setBacktestPeriod("2024-05-20 09:00:00", "2024-05-20 15:00:00");

        // 4. 启动回测
        backtest_engine.run();

        // 5. 回测结束后，打印最终结果
        auto final_fund = account.getFundInfo();
        std::cout << "\n--- Backtest Results ---" << std::endl;
        std::cout << "Final Total Asset: " << final_fund.total_asset << std::endl;
        std::cout << "Final Available: " << final_fund.available << std::endl;
        std::cout << "Final Fee: " << final_fund.fee << std::endl;
        std::cout << "Final Position PnL: " << final_fund.position_pnl << std::endl;

    }
    else {
        // --- 实盘/模拟盘模式 ---
        std::cout << "Starting Live Trading..." << std::endl;

        market_service.addDataSource(std::make_unique<DceDataSource>());
        market_service.addDataSource(std::make_unique<CzceDataSource>());
        market_service.start();

        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            event_system.publish(MarketUpdateEvent{});
        }

        market_service.stop();
    }

    return 0;
}

int sc() {
    EventSystem event_system;

    // --- 初始化 ---
    // 1. 初始化账户
    auto& account = Account::Instance();
    account.initialize(&event_system);

    // 2. 初始化策略
    auto& strategy = Strategy::Instance();
    strategy.initialize(&event_system);

    // 3. 初始化市场服务
    auto& market_service = MarketService::Instance();
    market_service.initialize(&event_system); // 注入事件系统
    market_service.addDataSource(std::make_unique<DceDataSource>());
    market_service.addDataSource(std::make_unique<CzceDataSource>());

    // --- 订阅事件 ---
    // 账户订阅订单更新事件
    event_system.subscribe<OrderUpdateEvent>("ORDER_UPDATE",
        [](const OrderUpdateEvent& e) { Account::Instance().on_order_update(e); });

    // 账户订阅交易信号事件
    event_system.subscribe<TradeSignalEvent>("TRADE_SIGNAL",
        [](const TradeSignalEvent& e) {
            Account::Instance().execute_order(e.instrument, e.direction, e.price, e.volume);
        });

    // --- 启动 ---
    market_service.start();

    // --- 主循环 ---
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 模拟：每秒获取一次数据，触发市场更新事件
        // 在实际应用中，这个触发应该由 MarketService 内部的定时器或数据源推送完成
        event_system.publish(MarketUpdateEvent{});
    }

    // --- 清理 ---
    market_service.stop();

    return 0;
}

int main() {
    // 解决 Windows 控制台中文乱码
    system("chcp 65001");
    get_one_second_random_quotes();
    MysqlDB db;
    // 创建分析类实例
    MarketAnalyzer analyzer;
    auto& acc = Account::Instance();
    auto positons = acc.getAllPositions();
    while (true) {
        // 1. 生成行情 → 写入缓存
        GenerateRandomDceQuote();
        GenerateRandomCzceQuote();

        // 2. 清屏 + 打印行情
        system("cls");
        PrintMarketCache();

        // 3. 分析模块：从缓存读取并统计
        analyzer.printAnalysis();
        
		db.getone();
        // 每秒刷新
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }

    return 0;
}