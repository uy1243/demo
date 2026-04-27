#pragma once
#include <string>

// 订单状态
enum OrderStatus {
    ORDER_CREATED    = '0',  // 已创建
    ORDER_INSERTING  = '1',  // 报单中
    ORDER_ACCEPTED   = '2',  // 已受理
    ORDER_PART_TRADED= '3',  // 部分成交
    ORDER_ALL_TRADED = '4',  // 全部成交
    ORDER_CANCELLED  = '5',  // 已撤单
    ORDER_REJECTED   = '6',  // 拒单
    ORDER_UNKNOWN    = '9'
};

// 订单信息
struct OrderInfo {
    std::string order_ref;
    std::string instrument_id;
    char direction;
    char offset;
    double price;
    int volume;
    OrderStatus status;
    int front_id;
    int session_id;
    std::string order_sys_id;
};