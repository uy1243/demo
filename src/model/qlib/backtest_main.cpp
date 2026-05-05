// backtest_main.cpp
#include "backtesting/backtest_engine.h"
#include "data_source/qlib_data_source.h" // 引入 Qlib 数据源
#include "strategy/my_strategy.h" // 假设你的策略文件
#include "events/event_system.h"
#include "services/market_service.h"
#include <memory>
#include <iostream>
#include <cstdlib> // For system()

int main() {
    std::cout << "=== 启动集成 Qlib 的回测 ===" << std::endl;

    // 1. 准备 Qlib 数据
    std::string instruments_str = "m2509 SR2509"; // 用空格分隔的合约列表
    std::string start_date = "2024-01-01";
    std::string end_date = "2024-12-31";
    std::string qlib_output_file = "qlib_factors_output.csv";

    // 调用 Python 脚本生成数据
    std::string cmd = "python qlib_data_service.py --instruments " + instruments_str +
        " --start_date " + start_date + " --end_date " + end_date +
        " --output_file " + qlib_output_file;

    std::cout << "执行 Qlib 数据服务: " << cmd << std::endl;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "执行 Python 脚本失败，返回码: " << ret << std::endl;
        return -1;
    }
    std::cout << "Python 脚本执行完成。" << std::endl;

    // 2. 初始化 C++ 组件
    EventSystem event_system;
    MarketService market_service;
    BacktestEngine backtest_engine(&event_system, &market_service);

    // 3. 添加 Qlib 数据源
    auto qlib_source = std::make_unique<QlibDataSource>(qlib_output_file);
    qlib_source->connect();
    backtest_engine.addDataSource(std::move(qlib_source));

    // 4. 添加其他数据源 (如 MySQL, Mock)
    // ... (保持原有的数据源添加逻辑) ...

    // 5. 设置回测周期
    backtest_engine.setBacktestPeriod(start_date, end_date);

    // 6. 创建策略，并传入 Qlib 数据源以便策略可以查询因子/信号
    // 假设你的策略构造函数接受一个 QlibDataSource 的指针或共享指针
    // auto strategy = std::make_shared<MyStrategy>(&event_system, /*...*/, qlib_data_source_ptr_if_needed);
    // event_system.subscribe(MarketUpdateEvent::EventType(), strategy);

    // 7. 运行回测
    backtest_engine.run();

    std::cout << "=== 回测结束 ===" << std::endl;
    return 0;
}