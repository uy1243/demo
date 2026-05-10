// datasources/mysql_db/tick_data_persister.h
#pragma once

#include "../../common/types.h"
#include <string>
#include <vector>
#include <memory>
#include <map> // 引入 map 缓存 PreparedStatement

namespace sql {
    class Connection;
    class PreparedStatement;
}

class TickDataPersister {
public:
    TickDataPersister(const std::string& host, int port,
        const std::string& user, const std::string& password,
        const std::string& database);
    ~TickDataPersister();

    bool initialize();

    // 新增：支持指定合约保存
    bool save(const TickData& tick);
    bool saveBatch(const std::vector<TickData>& ticks);

    // 新增：手动清理旧合约表
    bool dropTable(const std::string& instrument);

    bool isConnected() const;

private:
    std::string host_, user_, password_, database_;
    int port_;
    sql::Connection* connection_;

    // 缓存预处理语句，避免重复创建：Key为表名
    std::map<std::string, std::unique_ptr<sql::PreparedStatement>> statement_cache_;

    bool connect();
    void disconnect();

    // 核心逻辑变更
    std::string getTableName(const std::string& instrument); // 获取标准化工号表名
    bool createTableIfNotExists(const std::string& table_name); // 支持指定表名创建
    std::unique_ptr<sql::PreparedStatement> getStatement(const std::string& table_name); // 获取或创建预编译语句
};