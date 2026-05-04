// state_machine/account.cpp
#include "account.h"
#include "../events/event_system.h" // 包含 OrderUpdateEvent 定义
#include <chrono>
#include <iostream>

void Account::initialize(EventSystem* event_sys, ITrader* trader) {
    event_system_ = event_sys;
    trader_ = trader;
}

Account& Account::Instance() {
    static Account ins;
    return ins;
}

std::string Account::genOrderId() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return "ORD" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

Position* Account::getPosition(const std::string& inst, Direction dir) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string key = inst + (dir == Direction::LONG ? "_L" : "_S");
    if (positions_.find(key) == positions_.end()) {
        Position p;
        p.instrument = inst;
        p.dir = dir;
        positions_[key] = p;
    }
    return &positions_[key];
}

void Account::updateFund(double fee, double frozen_delta) {
    std::lock_guard<std::mutex> lock(mtx_);
    fund_.fee += fee;
    fund_.frozen += frozen_delta;
    fund_.available = fund_.total_asset - fund_.frozen - fund_.position_pnl;
}

void Account::updatePosition(const std::string& inst, Direction dir, int vol, double price) {
    std::lock_guard<std::mutex> lock(mtx_);
    Position* pos = getPosition(inst, dir);
    int old_vol = pos->volume;
    double old_cost = pos->avg_price * old_vol;

    pos->volume += vol;
    if (pos->volume > 0) {
        pos->avg_price = (old_cost + price * vol) / pos->volume;
    }
}

std::string Account::execute_order(const std::string& inst, Direction dir, double price, int vol) {
    if (!trader_) {
        std::cerr << "[Account] Trader not initialized!" << std::endl;
        return "";
    }

    std::string local_order_id = genOrderId();

    // 创建本地订单记录
    {
        std::lock_guard<std::mutex> lock(mtx_);
        Order o;
        o.order_id = local_order_id;
        o.instrument = inst;
        o.dir = dir;
        o.price = price;
        o.volume = vol;
        o.status = OrderStatus::SUBMITTING; // 本地认为正在提交
        orders_[local_order_id] = o;
    }

    // 发送真实订单请求
    OrderRequest req;
    req.instrument = inst;
    req.direction = dir;
    req.price = price;
    req.volume = vol;
    req.custom_order_ref = local_order_id;

    OrderResponse resp = trader_->placeOrder(req);

    if (resp.status == OrderStatus::REJECTED) {
        // 如果本地就失败，更新状态
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = orders_.find(local_order_id);
        if (it != orders_.end()) {
            it->second.status = OrderStatus::REJECTED;
        }
        std::cerr << "[Account] Order rejected locally: " << resp.error_msg << std::endl;
        return ""; // 或者返回 local_order_id，但标记为失败
    }

    return local_order_id;
}

void Account::execute_cancel(const std::string& order_id) {
    if (!trader_) {
        std::cerr << "[Account] Trader not initialized!" << std::endl;
        return;
    }

    CancelRequest req;
    req.order_id = order_id;

    trader_->cancelOrder(req);
}

void Account::on_order_update(const OrderUpdateEvent& event) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = orders_.find(event.order_id);
    if (it != orders_.end()) {
        it->second.status = event.new_status;
        it->second.filled = event.filled_volume;
        it->second.exchange_order_id = event.exchange_order_id; // 假设事件里有这个字段

        if (event.new_status == OrderStatus::FILLED || event.new_status == OrderStatus::PART_FILLED) {
            // 更新持仓
            updatePosition(it->second.instrument, it->second.dir, event.filled_volume, event.avg_fill_price);
            // TODO: 解冻已成交部分的资金
            double unfreeze_amount = event.avg_fill_price * event.filled_volume * 0.1; // 示例
            updateFund(0, -unfreeze_amount);
        }
        else if (event.new_status == OrderStatus::CANCELED) {
            // 订单被撤销，解冻全部资金
            double unfreeze_total = it->second.price * it->second.volume * 0.1; // 示例保证金
            updateFund(0, -unfreeze_total);
        }
        // TODO: 根据需要更新资金...
    }
}

std::vector<Order> Account::getAllOrders() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<Order> res;
    for (const auto& p : orders_) res.push_back(p.second);
    return res;
}

std::vector<Position> Account::getAllPositions() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<Position> res;
    for (const auto& p : positions_) res.push_back(p.second);
    return res;
}

AccountFund Account::getFundInfo() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return fund_;
}