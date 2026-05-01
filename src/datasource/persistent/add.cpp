#include "mysql_db.hpp"
#include "includes/market_data.hpp"  // 你统一的Tick结构体

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
    db.execute(sql);
}