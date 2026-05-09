// data_sources/mysql_data_source.h
#pragma once
#include "../market_types.h"
#include <string>
#include <vector>
#include <map>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

class MysqlDataSource : public IDataSource {
public:
    MysqlDataSource(const std::string& host, int port,
        const std::string& user, const std::string& password,
        const std::string& database);

    ~MysqlDataSource(); // 添加析构函数以清理资源

    std::string getName() const override { return "MYSQL"; }
    std::vector<TickData> fetchQuotes(const std::string& instrument) override;

    std::multimap<std::string, TickData> fetchHistoricalData(
        const std::string& instrument,
        const std::string& start_time,
        const std::string& end_time
    );

    bool saveTickDataToDB(const TickData& tick);
    bool saveTickDataBatchToDB(const std::vector<TickData>& ticks);
    bool createTickDataTable();  // 创建用于存储tick数据的表

private:
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;

    sql::mysql::MySQL_Driver* driver_;      // MySQL 驱动对象
    sql::Connection* connection_;           // 数据库连接对象

    void connect();                         // 建立连接
    void disconnect();                      // 断开连接
};