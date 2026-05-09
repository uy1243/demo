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

bool MysqlDataSource::createTickDataTable() {
    if (!connection_) {
        std::cerr << "[MYSQL] 未连接到数据库，无法创建表" << std::endl;
        return false;
    }

    try {
        // 创建ticks表来存储tick数据
        std::string create_table_sql = R"(
            CREATE TABLE IF NOT EXISTS ticks (
                id BIGINT AUTO_INCREMENT PRIMARY KEY,
                instrument VARCHAR(20) NOT NULL,
                source VARCHAR(20) DEFAULT 'UNKNOWN',
                time DATETIME NOT NULL,
                last DECIMAL(15,4) NOT NULL,
                open DECIMAL(15,4) NOT NULL,
                high DECIMAL(15,4) NOT NULL,
                low DECIMAL(15,4) NOT NULL,
                bid1 DECIMAL(15,4),
                ask1 DECIMAL(15,4),
                bid2 DECIMAL(15,4),
                ask2 DECIMAL(15,4),
                bid3 DECIMAL(15,4),
                ask3 DECIMAL(15,4),
                bid4 DECIMAL(15,4),
                ask4 DECIMAL(15,4),
                bid5 DECIMAL(15,4),
                ask5 DECIMAL(15,4),
                bid_vol1 INT DEFAULT 0,
                ask_vol1 INT DEFAULT 0,
                bid_vol2 INT DEFAULT 0,
                ask_vol2 INT DEFAULT 0,
                bid_vol3 INT DEFAULT 0,
                ask_vol3 INT DEFAULT 0,
                bid_vol4 INT DEFAULT 0,
                ask_vol4 INT DEFAULT 0,
                bid_vol5 INT DEFAULT 0,
                ask_vol5 INT DEFAULT 0,
                volume BIGINT DEFAULT 0,
                open_interest BIGINT DEFAULT 0,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                INDEX idx_instrument_time (instrument, time),
                INDEX idx_time (time)
            ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci
        )";

        std::unique_ptr<sql::Statement> stmt(connection_->createStatement());
        stmt->execute(create_table_sql);

        std::cout << "[MYSQL] ticks表创建成功或已存在" << std::endl;
        return true;
    }
    catch (const sql::SQLException& e) {
        std::cerr << "[MYSQL] 创建表失败: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDataSource::saveTickDataToDB(const TickData& tick) {
    if (!connection_) {
        std::cerr << "[MYSQL] 未连接到数据库，无法保存数据" << std::endl;
        return false;
    }

    try {
        std::string insert_sql = R"(
            INSERT INTO ticks (
                instrument, source, time, last, open, high, low, 
                bid1, ask1, bid2, ask2, bid3, ask3, bid4, ask4, bid5, ask5,
                bid_vol1, ask_vol1, bid_vol2, ask_vol2, bid_vol3, ask_vol3, bid_vol4, ask_vol4, bid_vol5, ask_vol5,
                volume, open_interest
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        std::unique_ptr<sql::PreparedStatement> pstmt(connection_->prepareStatement(insert_sql));

        pstmt->setString(1, tick.instrument);
        pstmt->setString(2, tick.source);
        pstmt->setString(3, tick.time);
        pstmt->setDouble(4, tick.last);
        pstmt->setDouble(5, tick.open);
        pstmt->setDouble(6, tick.high);
        pstmt->setDouble(7, tick.low);
        pstmt->setDouble(8, tick.bid1);
        pstmt->setDouble(9, tick.ask1);
        pstmt->setDouble(10, tick.bid2);
        pstmt->setDouble(11, tick.ask2);
        pstmt->setDouble(12, tick.bid3);
        pstmt->setDouble(13, tick.ask3);
        pstmt->setDouble(14, tick.bid4);
        pstmt->setDouble(15, tick.ask4);
        pstmt->setDouble(16, tick.bid5);
        pstmt->setDouble(17, tick.ask5);
        pstmt->setInt(18, tick.bid_vol1);
        pstmt->setInt(19, tick.ask_vol1);
        pstmt->setInt(20, tick.bid_vol2);
        pstmt->setInt(21, tick.ask_vol2);
        pstmt->setInt(22, tick.bid_vol3);
        pstmt->setInt(23, tick.ask_vol3);
        pstmt->setInt(24, tick.bid_vol4);
        pstmt->setInt(25, tick.ask_vol4);
        pstmt->setInt(26, tick.bid_vol5);
        pstmt->setInt(27, tick.ask_vol5);
        pstmt->setInt64(28, tick.volume);
        pstmt->setInt64(29, tick.open_interest);

        pstmt->executeUpdate();

        return true;
    }
    catch (const sql::SQLException& e) {
        std::cerr << "[MYSQL] 保存Tick数据失败: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDataSource::saveTickDataBatchToDB(const std::vector<TickData>& ticks) {
    if (!connection_) {
        std::cerr << "[MYSQL] 未连接到数据库，无法批量保存数据" << std::endl;
        return false;
    }

    if (ticks.empty()) {
        return true; // 空数据视为成功
    }

    try {
        // 开始事务
        connection_->setAutoCommit(false);

        std::string insert_sql = R"(
            INSERT INTO ticks (
                instrument, source, time, last, open, high, low, 
                bid1, ask1, bid2, ask2, bid3, ask3, bid4, ask4, bid5, ask5,
                bid_vol1, ask_vol1, bid_vol2, ask_vol2, bid_vol3, ask_vol3, bid_vol4, ask_vol4, bid_vol5, ask_vol5,
                volume, open_interest
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )";

        std::unique_ptr<sql::PreparedStatement> pstmt(connection_->prepareStatement(insert_sql));

        for (const auto& tick : ticks) {
            pstmt->setString(1, tick.instrument);
            pstmt->setString(2, tick.source);
            pstmt->setString(3, tick.time);
            pstmt->setDouble(4, tick.last);
            pstmt->setDouble(5, tick.open);
            pstmt->setDouble(6, tick.high);
            pstmt->setDouble(7, tick.low);
            pstmt->setDouble(8, tick.bid1);
            pstmt->setDouble(9, tick.ask1);
            pstmt->setDouble(10, tick.bid2);
            pstmt->setDouble(11, tick.ask2);
            pstmt->setDouble(12, tick.bid3);
            pstmt->setDouble(13, tick.ask3);
            pstmt->setDouble(14, tick.bid4);
            pstmt->setDouble(15, tick.ask4);
            pstmt->setDouble(16, tick.bid5);
            pstmt->setDouble(17, tick.ask5);
            pstmt->setInt(18, tick.bid_vol1);
            pstmt->setInt(19, tick.ask_vol1);
            pstmt->setInt(20, tick.bid_vol2);
            pstmt->setInt(21, tick.ask_vol2);
            pstmt->setInt(22, tick.bid_vol3);
            pstmt->setInt(23, tick.ask_vol3);
            pstmt->setInt(24, tick.bid_vol4);
            pstmt->setInt(25, tick.ask_vol4);
            pstmt->setInt(26, tick.bid_vol5);
            pstmt->setInt(27, tick.ask_vol5);
            pstmt->setInt64(28, tick.volume);
            pstmt->setInt64(29, tick.open_interest);

            pstmt->addBatch();
        }

        pstmt->executeBatch();
        connection_->commit(); // 提交事务
        connection_->setAutoCommit(true);

        std::cout << "[MYSQL] 批量保存 " << ticks.size() << " 条tick数据成功" << std::endl;
        return true;
    }
    catch (const sql::SQLException& e) {
        std::cerr << "[MYSQL] 批量保存Tick数据失败，正在回滚: " << e.what() << std::endl;
        try {
            connection_->rollback(); // 回滚事务
            connection_->setAutoCommit(true);
        }
        catch (...) {}
        return false;
    }
}