#pragma once
#include <map>
#include <mutex>
#include "OrderDef.h"
#include "MySQLDB.h"

class OrderMachine {
public:
    static OrderMachine& instance() {
        static OrderMachine ins;
        return ins;
    }

    void set_db(MySQLDB* db) { this->db = db; }

    // 添加新订单
    void add_order(const OrderInfo& order) {
        std::lock_guard<std::mutex> lock(mtx);
        order_map[order.order_ref] = order;
        insert_order_to_db(order);
    }

    // 更新订单状态
    void update_status(const std::string& order_ref, OrderStatus status, const char* sys_id=nullptr) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = order_map.find(order_ref);
        if (it == order_map.end()) return;
        it->second.status = status;
        if (sys_id) it->second.order_sys_id = sys_id;
        update_order_db(it->second);
    }

    // 记录成交
    void add_trade(const std::string& order_ref, const std::string& instrument,
                   char dir, char offset, double price, int vol, const std::string& trade_id) {
        std::lock_guard<std::mutex> lock(mtx);
        insert_trade_db(order_ref, instrument, dir, offset, price, vol, trade_id);
        update_position(instrument, dir, offset, vol, price);
    }

    // 判断是否可撤单
    bool can_cancel(const std::string& order_ref) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = order_map.find(order_ref);
        if (it == order_map.end()) return false;
        return it->second.status < ORDER_ALL_TRADED && it->second.status != ORDER_CANCELLED;
    }

    // 防重下单
    bool is_order_exist(const std::string& order_ref) {
        std::lock_guard<std::mutex> lock(mtx);
        return order_map.count(order_ref) > 0;
    }

private:
    OrderMachine() : db(nullptr) {}
    std::map<std::string, OrderInfo> order_map;
    std::mutex mtx;
    MySQLDB* db;

    // DB 操作
    void insert_order_to_db(const OrderInfo& o);
    void update_order_db(const OrderInfo& o);
    void insert_trade_db(const std::string& ref, const std::string& instr,
                         char d, char o, double p, int v, const std::string& tid);
    void update_position(const std::string& instr, char dir, char offset, int vol, double price);
};