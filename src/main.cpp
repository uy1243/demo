#include "datasource/ctp/md_spi.h"
#include "datasource/ctp/trader_spi.h"
#include "utils/logger.h"
#include "datasource/market_data.h"
#include "state_machine/account.h"
#include "strategy/strategy.h"
#include "datasource/gui_operator/win32_auto.hpp"
#include "strategy/market_analyzer.h"
#include "datasource/mysql_db/db.hpp"
#include <windows.h>
// 👇 加上这一行，解决中文乱码
#pragma comment(linker, "/ENTRY:mainCRTStartup")

int main() {
    // 解决 Windows 控制台中文乱码
    system("chcp 65001");
    get_one_second_random_quotes();
    MysqlDB db;
    // 创建分析类实例
    MarketAnalyzer analyzer;
    auto& acc = Account::Instance();
    auto positons = acc.getAllPositions();
    while (true) {
        // 1. 生成行情 → 写入缓存
        GenerateRandomDceQuote();
        GenerateRandomCzceQuote();

        // 2. 清屏 + 打印行情
        system("cls");
        PrintMarketCache();

        // 3. 分析模块：从缓存读取并统计
        analyzer.printAnalysis();
        
		db.getone();
        // 每秒刷新
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }

    return 0;
}