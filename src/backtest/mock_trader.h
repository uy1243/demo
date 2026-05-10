// trading/mock_trader.h
#pragma once
#include "datasource/trader_interface.h"
#include "strategy/event_system.h"
#include <random>
#include <chrono>
#include <iostream>

class MockTrader : public ITrader {
public:
    MockTrader() : gen_(rd_()) {}

    std::string getName() const override { return "MOCK"; }

    void initialize(EventSystem* event_system) override {
        event_system_ = event_system;
    }

    OrderResponse placeOrder(const OrderRequest& req) override {
        OrderResponse resp;
        resp.local_order_ref = req.custom_order_ref;

        // 模拟下单成功
        resp.status = OrderStatus::SUBMITTING;
        resp.exchange_order_id = "EX" + req.custom_order_ref;

        // 模拟随机成交延迟
        std::uniform_int_distribution<int> delay_dist(0, 2);
        int delay = delay_dist(gen_);

        // 在另一个线程中模拟成交回报
        std::thread([this, req, delay]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay * 100));

            // 模拟成交
            std::uniform_real_distribution<double> fill_dist(0.8, 1.0);
            double fill_ratio = fill_dist(gen_);
            int filled_vol = static_cast<int>(req.volume * fill_ratio);

            OrderStatus status = (filled_vol >= req.volume)
                ? OrderStatus::FILLED
                : OrderStatus::PART_FILLED;

            if (event_system_) {
                // 修复：使用完整的构造函数调用，而不是初始化列表
                event_system_->publish(
                    OrderUpdateEvent(
                        req.custom_order_ref,           // order_id
                        "EX" + req.custom_order_ref,    // exchange_order_id
                        status,                         // new_status
                        filled_vol,                     // filled_volume
                        req.price + (std::uniform_real_distribution<double>(-0.5, 0.5)(gen_)), // avg_fill_price
                        ""                              // error_msg
                    )
                );
            }
            }).detach();

        return resp;
    }

    CancelResponse cancelOrder(const CancelRequest& req) override {
        CancelResponse resp;
        resp.local_order_ref = req.order_id;
        resp.success = true;

        if (event_system_) {
            // 修复：使用完整的构造函数调用
            event_system_->publish(
                OrderUpdateEvent(
                    req.order_id,   // order_id
                    "",             // exchange_order_id
                    OrderStatus::CANCELED,  // new_status
                    0,              // filled_volume
                    0.0,            // avg_fill_price
                    ""              // error_msg
                )
            );
        }

        return resp;
    }

    void start() override {
        std::cout << "[MockTrader] 模拟交易接口已启动" << std::endl;
    }

    void stop() override {
        std::cout << "[MockTrader] 模拟交易接口已停止" << std::endl;
    }

    virtual std::vector<Order> getAllOrders() const {
        return {}; // 这里需要实现从 CTP API 获取订单信息的逻辑
    }
    virtual std::vector<Position> getAllPositions() const {
        return {}; // 这里需要实现从 CTP API 获取持仓信息的逻辑
    }
    virtual AccountFund getFundInfo() const {
        return {}; // 这里需要实现从 CTP API 获取资金信息的逻辑
    }
private:
    EventSystem* event_system_ = nullptr;
    std::random_device rd_;
    std::mt19937 gen_;
};