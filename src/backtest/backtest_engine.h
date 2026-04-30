#pragma once
#include "backtest_data.hpp"
#include <vector>
#include <unordered_map>
#include <string>

class IStrategy {
public:
    virtual void on_bar(const Bar& bar) = 0;
};

class BacktestEngine {
public:
    std::vector<Bar> bars;
    IStrategy* strategy = nullptr;

    double cash = 100000;
    std::unordered_map<std::string, Position> positions;
    std::vector<Order> orders;
    std::vector<std::string> trade_log;

    void load_csv(const std::string& path);
    void run();
    void send_order(const std::string& instr, char dir, char off, double price, int vol);
    void match(const Bar& bar);
    void report();
};