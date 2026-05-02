#pragma once
#include <mysql/mysql.h>
#include <string>
#include <cstdio>

class MySQLDB {
public:
    MySQLDB() : conn(nullptr) {}
    ~MySQLDB() { close(); }

    bool connect(const char* host, const char* user, const char* pwd, const char* db, int port=3306) {
        conn = mysql_init(nullptr);
        if (!mysql_real_connect(conn, host, user, pwd, db, port, nullptr, 0)) {
            LOG_ERR("mysql connect failed: %s", mysql_error(conn));
            return false;
        }
        mysql_set_character_set(conn, "utf8mb4");
        return true;
    }

    void close() { if (conn) mysql_close(conn); conn = nullptr; }

    bool execute(const char* sql) {
        if (mysql_query(conn, sql)) {
            LOG_ERR("exec sql err: %s sql:%s", mysql_error(conn), sql);
            return false;
        }
        return true;
    }

    MYSQL_RES* query(const char* sql) {
        if (mysql_query(conn, sql)) return nullptr;
        return mysql_store_result(conn);
    }

private:
    MYSQL* conn;
};