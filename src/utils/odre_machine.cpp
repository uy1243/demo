#include "OrderMachine.h"
#include <string.h>
#include <stdio.h>

void OrderMachine::insert_order_to_db(const OrderInfo& o) {
    if (!db) return;
    char sql[1024];
    sprintf(sql, "INSERT INTO orders(order_ref,instrument_id,direction,offset,price,volume,status,front_id,session_id) "
                 "VALUES('%s','%s','%c','%c',%.2f,%d,'%c',%d,%d)",
            o.order_ref.c_str(), o.instrument_id.c_str(),
            o.direction, o.offset, o.price, o.volume, o.status,
            o.front_id, o.session_id);
    db->execute(sql);
}

void OrderMachine::update_order_db(const OrderInfo& o) {
    if (!db) return;
    char sql[1024];
    sprintf(sql, "UPDATE orders SET status='%c', order_sys_id='%s' WHERE order_ref='%s'",
            o.status, o.order_sys_id.c_str(), o.order_ref.c_str());
    db->execute(sql);
}

void OrderMachine::insert_trade_db(const std::string& ref, const std::string& instr,
                                   char d, char o, double p, int v, const std::string& tid) {
    if (!db) return;
    char sql[1024];
    sprintf(sql, "INSERT INTO trades(order_ref,instrument_id,direction,offset,price,volume,trade_id) "
                 "VALUES('%s','%s','%c','%c',%.2f,%d,'%s')",
            ref.c_str(), instr.c_str(), d, o, p, v, tid.c_str());
    db->execute(sql);
}

void OrderMachine::update_position(const std::string& instr, char dir, char offset, int vol, double price) {
    if (!db) return;
    char sel[512];
    sprintf(sel, "SELECT total_volume FROM positions WHERE instrument_id='%s' AND direction='%c'",
            instr.c_str(), dir);
    MYSQL_RES* res = db->query(sel);
    if (!res || mysql_num_rows(res) == 0) {
        char ins[512];
        sprintf(ins, "INSERT INTO positions(instrument_id,direction,total_volume,open_price) "
                     "VALUES('%s','%c',%d,%.2f)", instr.c_str(), dir, vol, price);
        db->execute(ins);
    } else {
        MYSQL_ROW row = mysql_fetch_row(res);
        int old = atoi(row[0]);
        mysql_free_result(res);
        char upd[512];
        sprintf(upd, "UPDATE positions SET total_volume=%d WHERE instrument_id='%s' AND direction='%c'",
                offset == '0' ? old+vol : old-vol, instr.c_str(), dir);
        db->execute(upd);
    }
}