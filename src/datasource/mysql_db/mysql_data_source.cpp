// data_sources/mysql_data_source.cpp
#include "mysql_data_source.h"
#include <iostream>
#include <sstream>
#include <algorithm>

MysqlDataSource::MysqlDataSource(
    const std::string& host, int port,
    const std::string& user, const std::string& password,
    const std::string& database)
    : host_(host), port_(port), user_(user), password_(password),
    database_(database), driver_(nullptr), connection_(nullptr) {

    try {
        // 初始化 MySQL 驱动
        driver_ = sql::mysql::get_mysql_driver_instance();
        if (!driver_) {
            std::cerr << "Failed to get MySQL driver instance" << std::endl;
            return;
        }

        // 建立连接
        connect();
    }
    catch (const sql::SQLException& e) {
        std::cerr << "MySQL initialization error: " << e.what()
            << " (Error code: " << e.getErrorCode()
            << ", SQLState: " << e.getSQLState() << ")" << std::endl;
    }
}

MysqlDataSource::~MysqlDataSource() {
    disconnect();
}

void MysqlDataSource::connect() {
    try {
        if (connection_) {
            disconnect();
        }

        // 构建连接字符串
        std::string conn_str = "tcp://" + host_ + ":" + std::to_string(port_);

        // 建立连接
        connection_ = driver_->connect(conn_str, user_, password_);

        if (!connection_) {
            std::cerr << "Failed to establish MySQL connection" << std::endl;
            return;
        }

        // 选择数据库
        connection_->setSchema(database_);

        std::cout << "Connected to MySQL database: " << database_
            << " at " << host_ << ":" << port_ << std::endl;
    }
    catch (const sql::SQLException& e) {
        std::cerr << "MySQL connection error: " << e.what()
            << " (Error code: " << e.getErrorCode()
            << ", SQLState: " << e.getSQLState() << ")" << std::endl;
        connection_ = nullptr;
    }
}

void MysqlDataSource::disconnect() {
    if (connection_) {
        try {
            connection_->close();
            delete connection_;
            connection_ = nullptr;
            std::cout << "Disconnected from MySQL database" << std::endl;
        }
        catch (const sql::SQLException& e) {
            std::cerr << "MySQL disconnection error: " << e.what() << std::endl;
        }
    }
}

std::vector<TickData> MysqlDataSource::fetchQuotes(const std::string& instrument) {
    std::vector<TickData> res;

    if (!connection_) {
        std::cerr << "MySQL connection is not available" << std::endl;
        return res;
    }

    try {
        // 创建 SQL 语句对象
        sql::Statement* stmt = connection_->createStatement();

        // 构建查询语句
        std::string query = "SELECT open, high, low, close, volume, tdate, instrument "
            "FROM " + database_ + ".a09_4day_realtime "
            "WHERE instrument = '" + instrument + "' "
            "ORDER BY tdate DESC LIMIT 100";

        // 执行查询
        sql::ResultSet* result = stmt->executeQuery(query);

        // 遍历结果集
        while (result->next()) {
            TickData tick;
            tick.instrument = instrument;
            tick.source = getName();

            tick.open = result->getDouble("open");
            tick.high = result->getDouble("high");
            tick.low = result->getDouble("low");
            tick.last = result->getDouble("close");  // 使用收盘价作为最新价
            tick.volume = result->getInt64("volume");
            tick.time = result->getString("tdate");

            // 估算买卖价差（如果没有实际数据）
            tick.bid1 = tick.last - 0.5;
            tick.ask1 = tick.last + 0.5;

            res.push_back(tick);
        }

        // 清理资源
        delete result;
        delete stmt;
    }
    catch (const sql::SQLException& e) {
        std::cerr << "Error fetching quotes for " << instrument << ": "
            << e.what() << " (Error code: " << e.getErrorCode() << ")" << std::endl;
    }

    return res;
}

std::multimap<std::string, TickData> MysqlDataSource::fetchHistoricalData(
    const std::string& instrument,
    const std::string& start_time,
    const std::string& end_time
) {
    std::multimap<std::string, TickData> sorted_ticks;

    if (!connection_) {
        std::cerr << "MySQL connection is not available" << std::endl;
        return sorted_ticks;
    }

    try {
        std::cout << "fetchHistoricalData: instrument=" << instrument
            << ", database=" << database_
            << ", start_time=" << start_time
            << ", end_time=" << end_time << std::endl;

        // 使用预处理语句防止 SQL 注入
        sql::PreparedStatement* pstmt = connection_->prepareStatement(
            "SELECT open, high, low, close, volume, tdate, instrument "
            "FROM " + database_ + ".a09_4day_realtime "
            "WHERE instrument = ? AND tdate >= ? AND tdate <= ? "
            "ORDER BY tdate ASC"
        );

        // 绑定参数
        pstmt->setString(1, instrument);
        pstmt->setString(2, start_time);
        pstmt->setString(3, end_time);

        // 执行查询
        sql::ResultSet* result = pstmt->executeQuery();

        // 遍历结果集
        while (result->next()) {
            TickData tick;
            tick.instrument = instrument;
            tick.source = getName();

            tick.open = result->getDouble("open");
            tick.high = result->getDouble("high");
            tick.low = result->getDouble("low");
            tick.last = result->getDouble("close");
            tick.volume = result->getInt64("volume");
            tick.time = result->getString("tdate");

            // 估算买卖价差
            tick.bid1 = tick.last - 0.5;
            tick.ask1 = tick.last + 0.5;

            // 插入到 multimap，按时间排序
            sorted_ticks.emplace(tick.time, tick);
        }

        // 清理资源
        delete result;
        delete pstmt;

        std::cout << "Fetched " << sorted_ticks.size()
            << " records for " << instrument << std::endl;
    }
    catch (const sql::SQLException& e) {
        std::cerr << "Error fetching historical data for " << instrument
            << " from " << start_time << " to " << end_time
            << ": " << e.what()
            << " (Error code: " << e.getErrorCode()
            << ", SQLState: " << e.getSQLState() << ")" << std::endl;
    }

    return sorted_ticks;
}