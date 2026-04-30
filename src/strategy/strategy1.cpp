#pragma once
#include "backtest_engine.hpp"
#include <deque>

class MaStrategy : public IStrategy {
public:
    BacktestEngine* bt;
    std::deque<double> ma5, ma10;

    void on_bar(const Bar& bar) override {
        ma5.push_back(bar.close);
        ma10.push_back(bar.close);

        if (ma5.size() > 5) ma5.pop_front();
        if (ma10.size() > 10) ma10.pop_front();

        if (ma5.size() < 5 || ma10.size() < 10) return;

        double a5 = 0, a10 = 0;
        for (auto v : ma5) a5 += v; a5 /= 5;
        for (auto v : ma10) a10 += v; a10 /= 10;

        if (a5 > a10) {
            bt->send_order(bar.instrument, '1', '0', bar.close, 1);
        }
        else if (a5 < a10) {
            bt->send_order(bar.instrument, '0', '0', bar.close, 1);
        }
    }
};