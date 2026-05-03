#include <iostream>
#include <vector>
#include <thread>
#include "market_data.h"

int main() {
    std::cout << "=== 行情程序（每秒自动刷新）===" << std::endl << std::endl;

    while (true) {
        // 1. 每秒生成一次最新数据 → 自动存入缓存
        auto dce = get_dce_quotes_random("m");
        auto czce = get_czce_quotes_random("SR");

        // 2. 清屏（可选，看起来像实时行情终端）
        system("cls");

        // 3. 打印大商所
        std::cout << "=== 大商所 DCE 豆粕 （实时刷新）===" << std::endl;
        for (auto& q : dce) {
            printf("%-10s 最新:%.2f 买一:%.2f 卖一:%.2f 量:%d 时间:%s\n",
                q.instrument.c_str(), q.last, q.bid1, q.ask1, q.volume, q.time.c_str());
        }

        // 4. 打印郑商所
        std::cout << "\n=== 郑商所 CZCE 白糖 ===" << std::endl;
        for (auto& q : czce) {
            printf("%-10s 最新:%.2f 买一:%.2f 卖一:%.2f 量:%d 时间:%s\n",
                q.instrument.c_str(), q.last, q.bid1, q.ask1, q.volume, q.time.c_str());
        }

        // 5. 其他地方可以直接拿缓存，不影响速度
        // auto cached_dce = get_dce_cache();

        // 6. 每秒一次
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}