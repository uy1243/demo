#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <mysqlx/xdevapi.h>
#include "log/logger.h"

class MySQLDB
{
public:
    MySQLDB() = default;
    bool connect(const std::string& host, const std::string& user, const std::string& pwd, const std::string& dbname)
    {
        try {
            // 1. 先关闭旧连接（如果有）
            close();

            // 2. 尝试创建新会话
            sess = new mysqlx::Session(host, 33060, user, pwd);

            // 3. 执行操作
            sess->sql("USE " + dbname).execute();
            sess->sql("SET NAMES utf8mb4").execute();

            LOG_INFO("✅ MySQL 连接成功");
            return true;
        }
        catch (const mysqlx::Error& e) {
            LOG_ERR("❌ MySQL 连接失败: ", e.what());

            // 【关键修复】：连接失败后，必须确保 sess 为空，防止野指针
            if (sess) {
                delete sess;
                sess = nullptr;
            }
            return false;
        }
    }

    void close()
    {
        if (sess) {
            try {
                sess->close();
                delete sess;
                sess = nullptr;
            }
            catch (...) {}
        }
    }

    void insert_tick(
        const std::string& instrument,
        double last,
        double bid1,
        double ask1,
        int volume
    ) {
        if (!sess) return;

        try {
            sess->sql(R"(
                INSERT INTO tick_data (instrument, last_price, bid1, ask1, volume)
                VALUES (?, ?, ?, ?, ?)
            )")
                .bind(instrument, last, bid1, ask1, volume)
                .execute();
        }
        catch (const mysqlx::Error& e) {
            std::cout << "❌ 插入失败: " << e.what() << std::endl;
        }
    }

    // ===================== 已修复：查询函数 =====================
    std::vector<std::vector<std::string>> query(const std::string& sql)
    {
        std::vector<std::vector<std::string>> result;
        if (!sess) return result;

        try {
            mysqlx::SqlResult res = sess->sql(sql).execute();

            while (auto row = res.fetchOne()) {
                std::vector<std::string> cols;
                for (unsigned i = 0; i < row.colCount(); ++i) {
                    auto val = row.get(i);
                    if (val.isNull()) {
                        cols.emplace_back("");
                    }
                    else {
                        cols.push_back(val.get<std::string>());
                    }
                }
                result.push_back(cols);
            }
        }
        catch (const mysqlx::Error& e) {
            std::cout << "❌ 查询失败: " << e.what() << std::endl;
        }
        return result;
    }

    // ===================== 已修复：查询最后一条Tick =====================
    std::vector<std::string> query_last_tick(const std::string& instrument)
    {
        if (!sess) return {};

        std::string sql = R"(
            SELECT instrument, last_price, bid1, ask1, volume
            FROM tick_data
            WHERE instrument = ?
            ORDER BY id DESC LIMIT 1
        )";

        try {
            auto row = sess->sql(sql).bind(instrument).execute().fetchOne();
            if (!row) return {};

            std::vector<std::string> res;
            for (int i = 0; i < 5; ++i) {
                auto val = row.get(i);
                if (val.isNull()) {
                    res.push_back("");
                }
                else {
                    res.push_back(val.get<std::string>());
                }
            }
            return res;
        }
        catch (const mysqlx::Error& e) {
            std::cout << "❌ 查询最新tick失败: " << e.what() << std::endl;
            return {};
        }
    }

    ~MySQLDB() {
        close();
    }

    // 禁止拷贝，防止崩溃
    MySQLDB(const MySQLDB&) = delete;
    MySQLDB& operator=(const MySQLDB&) = delete;

private:
    mysqlx::Session* sess = nullptr;
};