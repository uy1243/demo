#include "md_spi.h"
#include "trader_spi.h"

#ifdef _WIN32
#include <windows.h>
// 👇 加上这一行，解决中文乱码
#pragma comment(linker, "/ENTRY:mainCRTStartup")
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// ====================== SimNow 仿真环境标准配置 ======================
#define BROKER_ID  "9999"          // SimNow 固定为 9999
#define USER_ID    "simnow_client_test"   // 改成你自己的 SimNow 账号
#define PASSWORD   "0000000000000000"   // 改成你自己的 SimNow 密码

// SimNow 7x24 小时行情/交易地址（最稳定）
#define MD_ADDR    "tcp://182.254.243.31:40011"   // 行情
#define TD_ADDR    "tcp://182.254.243.31:40001"   // 交易
// ====================================================================

int main() {
#ifdef _WIN32
    // 解决 Windows 控制台中文乱码
    system("chcp 65001");
#endif
    // 启动行情
    CMdSpi md(BROKER_ID, USER_ID, PASSWORD);
    md.connect(MD_ADDR);

#ifdef _WIN32
    Sleep(1000);
#else
    usleep(1000000);
#endif

    // 订阅合约（SimNow 支持所有合约）
    const char* instrs[] = { "rb2510", "p2509", "ma2509", "cu2509", "au2506" };
    md.subscribe(instrs, sizeof(instrs) / sizeof(const char*));

    // 启动交易
    CTraderSpi td(BROKER_ID, USER_ID, PASSWORD);
    td.connect(TD_ADDR);

#ifdef _WIN32
    Sleep(1000);
#else
    usleep(1000000);
#endif

    // 查询持仓
    td.query_position();

    // 保持程序运行
    while (true) {
#ifdef _WIN32
        Sleep(1000);
#else
        usleep(1000000);
#endif
    }

    return 0;
}