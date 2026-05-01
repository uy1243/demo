#pragma once
#include <string>

// 统一行情结构（CTP、新浪、回测 全都用这个！）
struct TickData
{
    std::string instrument_id;  // 合约
    double last_price = 0.0;    // 最新价
    double bid_price1 = 0.0;    // 买一
    double ask_price1 = 0.0;    // 卖一
    int volume = 0;             // 量
    std::string time;           // 时间
};