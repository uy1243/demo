#include "backtest_engine.hpp"
#include "strategy.hpp"
#include <iostream>

int main() {
    BacktestEngine bt;
    bt.load_csv("data.csv");  // 回测数据

    MaStrategy s;
    s.bt = &bt;
    bt.strategy = &s;

    bt.run();
    return 0;
}