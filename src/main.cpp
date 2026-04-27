#include "MdSpi.h"
#include "TraderSpi.h"
#include <unistd.h>

// ====================== 你只需要改这里 ======================
#define BROKER_ID  "期货公司经纪代码"
#define USER_ID    "你的资金账号"
#define PASSWORD   "你的交易密码"
#define MD_ADDR    "tcp://行情地址:端口"
#define TD_ADDR    "tcp://交易地址:端口"
// ==========================================================

int main() {
    // 启动行情
    CMdSpi md(BROKER_ID, USER_ID, PASSWORD);
    md.connect(MD_ADDR);
    sleep(1);

    // 订阅合约
    const char* instrs[] = {"p2509", "rb2510", "ma2509"};
    md.subscribe(instrs, sizeof(instrs)/sizeof(const char*));

    // 启动交易
    CTraderSpi td(BROKER_ID, USER_ID, PASSWORD);
    td.connect(TD_ADDR);
    sleep(2);

    // 查询持仓
    td.query_position();

    // 挂起线程
    while (true) {
        sleep(1);
    }
    return 0;
}