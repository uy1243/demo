#include "account.h"
#include <chrono>
#include <random>
#include <iomanip>
#include <sstream>

Account& Account::Instance() {
    static Account ins;
    return ins;
}

std::string Account::genOrderId() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return "ORD" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

Position* Account::getPosition(const std::string& inst, Direction dir) {
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
    fund_.fee += fee;
    fund_.frozen += frozen_delta;
    fund_.available = fund_.total_asset - fund_.frozen - fund_.position_pnl;
}

// 买开
std::string Account::buy(const std::string& inst, double price, int vol) {
    std::lock_guard<std::mutex> lock(mtx_);
    Order o;
    o.order_id = genOrderId();
    o.instrument = inst;
    o.dir = Direction::LONG;
    o.price = price;
    o.volume = vol;
    o.status = OrderStatus::PENDING;
    orders_[o.order_id] = o;

    updateFund(1.0, price * vol * 0.1);
    return o.order_id;
}

// 卖开
std::string Account::sell(const std::string& inst, double price, int vol) {
    std::lock_guard<std::mutex> lock(mtx_);
    Order o;
    o.order_id = genOrderId();
    o.instrument = inst;
    o.dir = Direction::SHORT;
    o.price = price;
    o.volume = vol;
    o.status = OrderStatus::PENDING;
    orders_[o.order_id] = o;

    updateFund(1.0, price * vol * 0.1);
    return o.order_id;
}

void Account::cancelOrder(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (orders_.count(order_id)) {
        orders_[order_id].status = OrderStatus::CANCELED;
        updateFund(0, -orders_[order_id].price * orders_[order_id].volume * 0.1);
    }
}

// 模拟撮合
void Account::matchOrders(const MarketCache& cache) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& [id, o] : orders_) {
        if (o.status != OrderStatus::PENDING && o.status != OrderStatus::PART_FILLED)
            continue;

        auto it = cache.find(o.instrument);
        if (it == cache.end()) continue;

        double px = it->second.last;
        if (fabs(px - o.price) < 5) {
            int trade_vol = o.volume - o.filled;
            o.filled += trade_vol;

            Position* pos = getPosition(o.instrument, o.dir);
            pos->volume += trade_vol;
            pos->avg_price = (pos->avg_price * pos->volume + o.price * trade_vol) / (pos->volume + trade_vol);

            if (o.filled >= o.volume)
                o.status = OrderStatus::FILLED;
            else
                o.status = OrderStatus::PART_FILLED;

            updateFund(0, -o.price * trade_vol * 0.1);
        }
    }
}

// 更新持仓盈亏
void Account::updatePnL(const MarketCache& cache) {
    std::lock_guard<std::mutex> lock(mtx_);
    fund_.position_pnl = 0;

    for (auto& [key, pos] : positions_) {
        auto it = cache.find(pos.instrument);
        if (it == cache.end()) continue;

        double pnl = 0;
        if (pos.dir == Direction::LONG)
            pnl = (it->second.last - pos.avg_price) * pos.volume * 10;
        else
            pnl = (pos.avg_price - it->second.last) * pos.volume * 10;

        pos.pnl = pnl;
        fund_.position_pnl += pnl;
    }

    fund_.total_asset = 1000000 + fund_.position_pnl - fund_.fee;
    updateFund(0, 0);
}

std::vector<Order> Account::getAllOrders() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<Order> res;
    for (auto& p : orders_) res.push_back(p.second);
    return res;
}

std::vector<Position> Account::getAllPositions() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<Position> res;
    for (auto& p : positions_) res.push_back(p.second);
    return res;
}

AccountFund Account::getFundInfo() {
    std::lock_guard<std::mutex> lock(mtx_);
    return fund_;
}