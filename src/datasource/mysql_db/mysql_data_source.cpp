// data_sources/mysql_data_source.cpp
#include "mysql_data_source.h"
#include <iostream>
#include <sstream>
#include <algorithm>

MysqlDataSource::MysqlDataSource(
    const std::string& host, int port,
    const std::string& user, const std::string& password,
    const std::string& database)
    : host_(host), port_(port), user_(user), password_(password), database_(database) {
}

Session MysqlDataSource::getSession() {
    try {
        std::string conn_str = host_ + ":" + std::to_string(port_);
        return Session(conn_str, user_, password_);
    }
    catch (const mysqlx::abi2::r0::Error& e) {
        std::cerr << "Failed to connect to MySQL: " << e.what() << std::endl;
        throw; // 或者返回一个无效的 Session
    }
}

std::vector<TickData> MysqlDataSource::fetchQuotes(const std::string& instrument) {
    // 为了兼容 IDataSource 接口，我们可以获取最近的数据
    // 但在回测中，我们会使用 fetchHistoricalData
    std::vector<TickData> res;
    try {
        auto session = getSession();
        auto db = session.getSchema(database_);
        auto table = db.getTable("a09_4day_realtime"); // 假设表名是固定的

        // 这里需要根据你的表结构调整 SQL 查询
        // 假设表中有 instrument, timestamp, open, high, low, close, volume 等字段
        auto result = table.select("*").where("instrument = :inst").bind("inst", instrument).execute();

        Row row;
        while ((row = result.fetchOne())) {
            TickData tick;
            tick.instrument = instrument;
            tick.source = getName();
            // 假设列顺序是 open, high, low, close, volume, time, instrument
            // 请根据你的实际表结构调整
            tick.last = row[3].get<double>(); // close 作为 last
            tick.open = row[0].get<double>();
            tick.high = row[1].get<double>();
            tick.low = row[2].get<double>();
            tick.volume = static_cast<long long>(row[4].get<int>());
            tick.time = row[5].get<std::string>(); // 假设时间是第6列

            // 其他字段如 bid1, ask1, open_interest 需要估算或留空
            tick.bid1 = tick.last - 0.5; // 示例
            tick.ask1 = tick.last + 0.5; // 示例

            res.push_back(tick);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error fetching quotes for " << instrument << ": " << e.what() << std::endl;
    }
    return res;
}

std::multimap<std::string, TickData> MysqlDataSource::fetchHistoricalData(
    const std::string& instrument,
    const std::string& start_time,
    const std::string& end_time
) {
    std::multimap<std::string, TickData> sorted_ticks; // 按时间排序
    try {
        auto session = getSession();
        auto db = session.getSchema(database_);
        auto table = db.getTable("a09_4day_realtime");

        // 修改SQL查询以适应你的表结构和时间字段
        // 假设时间字段名为 'timestamp'
        std::string sql_query = "SELECT open, high, low, close, volume, timestamp FROM " + database_ + ".a09_4day_realtime WHERE instrument = ? AND timestamp BETWEEN ? AND ? ORDER BY timestamp ASC";

        auto sql_res = session.sql(sql_query)
            .bind(instrument, start_time, end_time)
            .execute();

        Row row;
        while ((row = sql_res.fetchOne())) {
            TickData tick;
            tick.instrument = instrument;
            tick.source = getName();
            tick.last = row[3].get<double>(); // close
            tick.open = row[0].get<double>();
            tick.high = row[1].get<double>();
            tick.low = row[2].get<double>();
            tick.volume = static_cast<long long>(row[4].get<int>());
            tick.time = row[5].get<std::string>();

            tick.bid1 = tick.last - 0.5;
            tick.ask1 = tick.last + 0.5;

            sorted_ticks.emplace(tick.time, tick); // 插入 multimap，自动按键排序
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error fetching historical data for " << instrument
            << " from " << start_time << " to " << end_time
            << ": " << e.what() << std::endl;
    }
    return sorted_ticks;
}