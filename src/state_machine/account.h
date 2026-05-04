// state_machine/account.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "../common/types.h"
#include "../trading/trader_interface.h" // 新增

class Account {
public:
    static Account& Instance();
    void initialize(EventSystem* event_sys, ITrader* trader); // 新增 trader 参数

    // 执行下单（现在是真实下单）
    std::string execute_order(const std::string& inst, Direction dir, double price, int vol);
    void execute_cancel(const std::string& order_id);

    // 用于处理来自交易API的订单更新事件
    void on_order_update(const OrderUpdateEvent& event); // 假设你已在 event_system.h 中定义此事件

    // 查询接口
    std::vector<Order> getAllOrders() const;
    std::vector<Position> getAllPositions() const;
    AccountFund getFundInfo() const;

private:
    Account() = default;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, Order> orders_;
    std::unordered_map<std::string, Position> positions_;
    AccountFund fund_;
    EventSystem* event_system_ = nullptr;
    ITrader* trader_ = nullptr; // 新增

    std::string genOrderId();
    Position* getPosition(const std::string& inst, Direction dir);
    void updateFund(double fee, double frozen_delta);
    void updatePosition(const std::string& inst, Direction dir, int vol, double price);
};