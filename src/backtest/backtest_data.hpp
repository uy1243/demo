#pragma once
#include <string>
#include <vector>

// 1分钟K线
struct Bar {
    std::string instrument;
    std::string time;
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int volume = 0;
};

// 订单
struct Order {
    std::string instrument;
    char direction = '0'; // 1买 0卖
    char offset = '0';   // 开仓/平仓
    double price = 0;
    int volume = 0;
    bool filled = false;
};

// 持仓
struct Position {
    int long_pos = 0;
    int short_pos = 0;
    double cost = 0;
};