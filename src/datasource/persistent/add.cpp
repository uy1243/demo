#include "mysql_db.hpp"
#include "includes/market_data.hpp"  // 你统一的Tick结构体
#include <log/logger.h>

void save_tick_to_db(MySQLDB& db, const TickData& tick)
{
    char sql[1024];
    sprintf(sql,
        "INSERT INTO tick_data(instrument,last_price,bid1,ask1,volume) "
        "VALUES('%s',%.2f,%.2f,%.2f,%d)",
        tick.instrument_id.c_str(),
        tick.last_price,
        tick.bid_price1,
        tick.ask_price1,
        tick.volume
    );
	db.insert_tick(tick.instrument_id, tick.last_price, tick.bid_price1, tick.ask_price1, tick.volume);
}

void print_query_result(const std::vector<std::vector<std::string>>& result)
{
    if (result.empty()) {
        std::cout << "查询结果为空！" << std::endl;
        return;
    }

    // 遍历每一行
    for (const auto& row : result) {
        // 遍历每一列
        for (const auto& col : row) {
            std::cout << col << "\t"; // 用制表符分隔
        }
        std::cout << std::endl; // 一行结束换行
    }
}

//int main()
//{
//    MySQLDB db;
//	auto r = db.connect("localhost", "root", "Yu646010", "black");
//    if(!r) {
//        LOG_INFO( "连接数据库失败！");
//        return 1;
//	}
//    print_query_result(db.query("show tables;"));
//    return 0;
//}