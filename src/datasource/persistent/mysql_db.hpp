#pragma once
#include <string>
#include <iostream>
#include "mysqlx/xdevapi.h"

using namespace mysqlx;
using namespace std;

class MySQLDB
{
public:
    bool connect(const std::string& host, const std::string& user, const std::string& pwd, const std::string& dbname)
    {
        try {
            sess = new Session(host, 3306, user, pwd);
            sess->sql("USE " + dbname).execute();
            return true;
        }
        catch (const Error& e) {
            cout << "MySQL 连接失败: " << e.what() << endl;
            return false;
        }
    }

    // 插入行情数据
    void insert_tick(
        const std::string& instrument,
        double last,
        double bid1,
        double ask1,
        int volume
    ) {
        try {
            sess->sql(R"(
                INSERT INTO tick_data (instrument, last_price, bid1, ask1, volume)
                VALUES (?, ?, ?, ?, ?)
            )")
                .bind(instrument, last, bid1, ask1, volume)
                .execute();
        }
        catch (...) {}
    }

    ~MySQLDB() {
        if (sess) delete sess;
    }

private:
    Session* sess = nullptr;
};