#include "account_view.h"
#include <iostream>
#include <string>

using namespace std;

string statusStr(OrderStatus s) {
    switch (s) {
    case OrderStatus::PENDING: return "已报单";
    case OrderStatus::PART_FILLED: return "部分成交";
    case OrderStatus::FILLED: return "全部成交";
    case OrderStatus::CANCELED: return "已撤单";
    case OrderStatus::REJECTED: return "拒单";
    default: return "未知";
    }
}

string dirStr(Direction d) {
    return d == Direction::LONG ? "多" : "空";
}

void AccountView::printFund() {
    auto f = Account::Instance().getFundInfo();
    cout << "\n===== 账户资金 =====" << endl;
    printf("总权益: %.2lf | 可用: %.2lf | 冻结: %.2lf | 持仓盈亏: %.2lf\n",
        f.total_asset, f.available, f.frozen, f.position_pnl);
}

void AccountView::printOrders() {
    auto orders = Account::Instance().getAllOrders();
    cout << "\n===== 订单列表 =====" << endl;
    for (auto& o : orders) {
        printf("%s | %s | %s | 价:%.2f | %d/%d | %s\n",
            o.order_id.c_str(), o.instrument.c_str(), dirStr(o.dir).c_str(),
            o.price, o.filled, o.volume, statusStr(o.status).c_str());
    }
}

void AccountView::printPositions() {
    auto pos = Account::Instance().getAllPositions();
    cout << "\n===== 持仓信息 =====" << endl;
    for (auto& p : pos) {
        printf("%s | %s | 手数:%d | 均价:%.2f | 盈亏:%.2f\n",
            p.instrument.c_str(), dirStr(p.dir).c_str(),
            p.volume, p.avg_price, p.pnl);
    }
}

void AccountView::printAll() {
    printFund();
    printOrders();
    printPositions();
}