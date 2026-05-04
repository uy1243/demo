// main.cpp
#include "strategy/event_system.h"
#include "datasource/market_service.h"
#include "status/account.h"
#include "strategy/strategy.h"
#include "backtest/backtest_engine.h"
#include "datasource/mysql_db/mysql_data_source.h"
#include "utils/trading_hours.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <ctime>
#include "backtest/mock_trader.h"
#include "backtest/mock_data_source.h"

// 获取当前时间的辅助函数
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);

    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 打印分隔线
void printSeparator() {
    std::cout << "----------------------------------------" << std::endl;
}

// 打印回测结果
void printBacktestResults(const AccountFund& final_fund,
    const std::vector<Order>& all_orders,
    const std::vector<Position>& all_positions,
    const std::string& start_time,
    const std::string& end_time) {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "          回测结果报告" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "回测周期: " << start_time << " 至 " << end_time << std::endl;
    printSeparator();

    // 资金信息
    std::cout << "初始资金: 1,000,000.00" << std::endl;
    std::cout << "最终权益: " << std::fixed << std::setprecision(2)
        << final_fund.total_asset << std::endl;
    std::cout << "可用资金: " << final_fund.available << std::endl;
    std::cout << "持仓盈亏: " << final_fund.position_pnl << std::endl;
    std::cout << "总手续费: " << final_fund.fee << std::endl;

    double total_pnl = final_fund.total_asset - 1000000.0;
    double return_rate = (total_pnl / 1000000.0) * 100.0;
    std::cout << "总盈亏: " << total_pnl << std::endl;
    std::cout << "收益率: " << return_rate << "%" << std::endl;
    printSeparator();

    // 订单统计
    int total_orders = all_orders.size();
    int filled_orders = 0;
    int canceled_orders = 0;
    int rejected_orders = 0;

    for (const auto& order : all_orders) {
        switch (order.status) {
        case OrderStatus::FILLED:
        case OrderStatus::PART_FILLED:
            filled_orders++;
            break;
        case OrderStatus::CANCELED:
            canceled_orders++;
            break;
        case OrderStatus::REJECTED:
            rejected_orders++;
            break;
        default:
            break;
        }
    }

    std::cout << "订单统计:" << std::endl;
    std::cout << "  总订单数: " << total_orders << std::endl;
    std::cout << "  成交订单: " << filled_orders << std::endl;
    std::cout << "  撤单数: " << canceled_orders << std::endl;
    std::cout << "  拒单数: " << rejected_orders << std::endl;
    printSeparator();

    // 持仓信息
    if (!all_positions.empty()) {
        std::cout << "最终持仓:" << std::endl;
        for (const auto& pos : all_positions) {
            std::cout << "  " << pos.instrument
                << " " << (pos.dir == Direction::LONG ? "多" : "空")
                << " 数量:" << pos.volume
                << " 均价:" << pos.avg_price
                << " 浮动盈亏:" << pos.pnl << std::endl;
        }
    }
    else {
        std::cout << "最终持仓: 无" << std::endl;
    }
    printSeparator();

    // 最近10笔交易
    std::cout << "最近交易记录:" << std::endl;
    int count = 0;
    for (auto it = all_orders.rbegin();
        it != all_orders.rend() && count < 10;
        ++it, ++count) {
        const auto& order = *it;
        std::string status_str;
        switch (order.status) {
        case OrderStatus::FILLED: status_str = "已成交"; break;
        case OrderStatus::PART_FILLED: status_str = "部分成交"; break;
        case OrderStatus::CANCELED: status_str = "已撤单"; break;
        case OrderStatus::REJECTED: status_str = "已拒单"; break;
        case OrderStatus::PENDING: status_str = "待成交"; break;
        default: status_str = "未知"; break;
        }

        std::cout << "  " << order.order_id.substr(0, 8) << "..."
            << " " << order.instrument
            << " " << (order.dir == Direction::LONG ? "买入" : "卖出")
            << " 价格:" << order.price
            << " 数量:" << order.volume
            << " 已成交:" << order.filled
            << " 状态:" << status_str
            << std::endl;
    }
    std::cout << "==========================================" << std::endl;
}

int main0() {
    std::cout << "==========================================" << std::endl;
    std::cout << "      量化交易回测系统 v1.0" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "当前时间: " << getCurrentTime() << std::endl;
    std::cout << "当前地点: 江苏省苏州市" << std::endl;
    std::cout << "==========================================" << std::endl;

    // 创建事件系统
    EventSystem event_system;

    // 创建模拟交易接口（用于回测）
    auto mock_trader = std::make_unique<MockTrader>();

    // 初始化账户（初始资金100万）
    auto& account = Account::Instance();
    account.initialize(&event_system, mock_trader.get());

    // 初始化策略
    auto& strategy = Strategy::Instance();
    strategy.initialize(&event_system);

    // 初始化市场服务
    auto& market_service = MarketService::Instance();
    market_service.initialize(&event_system);

    // 设置事件订阅
    event_system.subscribe<OrderUpdateEvent>("ORDER_UPDATE",
        [](const OrderUpdateEvent& e) {
            Account::Instance().on_order_update(e);
        });

    event_system.subscribe<TradeSignalEvent>("TRADE_SIGNAL",
        [](const TradeSignalEvent& e) {
            Account::Instance().execute_order(e.instrument, e.direction, e.price, e.volume);
        });

    // 创建回测引擎
    BacktestEngine backtest_engine(&event_system, &market_service);

    // 添加 MySQL 数据源
    try {
        auto mysql_source = std::make_unique<MysqlDataSource>(
            "127.0.0.1",  // MySQL 主机
            33060,         // MySQL 端口
            "root",        // 用户名
            "Yu646010",    // 密码
            "black"        // 数据库名
        );
        backtest_engine.addDataSource(std::move(mysql_source));
        std::cout << "[Main] MySQL 数据源连接成功" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[Main] MySQL 连接失败: " << e.what() << std::endl;
        std::cerr << "[Main] 将使用模拟数据继续回测" << std::endl;

        // 如果 MySQL 连接失败，使用模拟数据源
        auto mock_source = std::make_unique<MockDataSource>();
        backtest_engine.addDataSource(std::move(mock_source));
    }

    // 设置回测周期
    std::string start_date = "2024-05-20 09:00:00";
    std::string end_date = "2024-05-24 15:00:00";
    backtest_engine.setBacktestPeriod(start_date, end_date);

    std::cout << "\n[Main] 开始回测..." << std::endl;
    std::cout << "[Main] 回测周期: " << start_date << " 至 " << end_date << std::endl;
    std::cout << "[Main] 初始资金: 1,000,000.00" << std::endl;
    std::cout << "[Main] 交易品种: m2509 (豆粕), SR2509 (白糖)" << std::endl;
    std::cout << "[Main] 策略: 趋势跟踪策略" << std::endl;
    printSeparator();

    // 记录开始时间
    auto start_time = std::chrono::high_resolution_clock::now();

    // 运行回测
    backtest_engine.run();

    // 记录结束时间
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    // 获取回测结果
    auto final_fund = account.getFundInfo();
    auto all_orders = account.getAllOrders();
    auto all_positions = account.getAllPositions();

    // 打印回测结果
    printBacktestResults(final_fund, all_orders, all_positions, start_date, end_date);

    std::cout << "\n回测耗时: " << duration.count() << " 秒" << std::endl;
    std::cout << "回测完成时间: " << getCurrentTime() << std::endl;
    std::cout << "==========================================" << std::endl;

    return 0;
}