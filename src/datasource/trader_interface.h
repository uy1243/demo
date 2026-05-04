// trading/trader_interface.h
#pragma once
#include "../common/types.h"
#include <string>
#include <memory>

struct OrderRequest {
    std::string instrument;
    Direction direction;
    double price;
    int volume;
    std::string custom_order_ref; // 本地订单ID，用于关联回报
};

struct OrderResponse {
    std::string local_order_ref;   // 本地关联的订单ID
    std::string exchange_order_id; // 交易所返回的订单ID
    OrderStatus status;            // 初始状态
    std::string error_msg;         // 错误信息
};

struct CancelRequest {
    std::string order_id; // 本地订单ID
};

struct CancelResponse {
    std::string local_order_ref;
    bool success;
    std::string error_msg;
};

class ITrader {
public:
    virtual ~ITrader() = default;
    virtual OrderResponse placeOrder(const OrderRequest& req) = 0;
    virtual CancelResponse cancelOrder(const CancelRequest& req) = 0;
    virtual void start() = 0; // 启动监听回报
    virtual void stop() = 0;
    virtual std::string getName() const = 0; // API 名称
};