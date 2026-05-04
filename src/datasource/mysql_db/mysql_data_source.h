// data_sources/mysql_data_source.h
#pragma once
#include "../market_types.h"
#include <string>
#include <vector>
#include <mysqlx/xdevapi.h>
#include <map> // 用于按时间排序数据

using namespace mysqlx;

class MysqlDataSource : public IDataSource {
public:
    MysqlDataSource(const std::string& host, int port, const std::string& user, const std::string& password, const std::string& database);

    std::string getName() const override { return "MYSQL"; }
    std::vector<TickData> fetchQuotes(const std::string& instrument) override; // 获取指定合约的历史数据

    // 专门用于回测的新方法：获取指定时间段内的所有数据
    std::multimap<std::string, TickData> fetchHistoricalData(
        const std::string& instrument,
        const std::string& start_time,
        const std::string& end_time
    );

private:
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;

    Session getSession();
};