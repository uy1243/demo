#include <iostream>
#include <mysqlx/xdevapi.h> // X DevAPI 的主头文件

using namespace std;
using namespace mysqlx; // 使用 mysqlx 命名空间
class MysqlDB {
public:	
	MysqlDB() = default;
	~MysqlDB() = default;
    void get_one()
    {
        try {
            Session sess("127.0.0.1:33060", "root", "Yu646010");
            Schema db = sess.getSchema("black");

            /*
            // --- 操作 Collection ---
            Collection coll = db.getCollection("test_collection");
            coll.add(R"({"name":"Charlie", "age": 35})").execute();

            // 关键修正：使用 DocResult 而不是 Result
            mysqlx::DocResult find_result = coll.find("age > 20").execute();
            mysqlx::DbDoc doc;
            std::cout << "\nReading from Collection:" << std::endl;
            while ((doc = find_result.fetchOne())) { // DocResult 有 fetchOne()
                std::cout << "Found document: " << doc << std::endl;
            }
            */
            // --- 操作 Table ---
            Table table = db.getTable("a09_4day_realtime");
            //table.insert("name", "age").values("Alice", 30).values("Bob", 25).execute();

            // 关键修正：使用 RowResult 而不是 Result
            //mysqlx::RowResult select_result = table.select("name", "age").where("age > 20").execute();
            mysqlx::RowResult select_result = table.select("*").execute();
            mysqlx::Row row;
            std::cout << "\nReading from Table:" << std::endl;
            while ((row = select_result.fetchOne())) { // RowResult 有 fetchOne()
                std::cout << "open: " << row[0] << ", max: " << row[1] << std::endl;
            }

        }
        catch (const mysqlx::abi2::r0::Error& err) {
            std::cerr << "MySQL Error: " << err.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Standard Error: " << e.what() << std::endl;
        }
    }

    void getone()
    {
        try {
            // 尝试使用 IP 地址
            auto config = mysqlx::SessionSettings("mysqlx://root:Yu646010@127.0.0.1:33060");
            mysqlx::Session session(config);

            // 执行一个简单的查询
            session.sql("USE black").execute();

            // 2. 执行 SELECT 查询
            auto result = session.sql("SELECT open FROM a09_4day_realtime").execute();
            std::vector<std::vector<float>> data;
            mysqlx::Row row;
            while ((row = result.fetchOne())) {
                std::vector<float> record;
                for (size_t i = 0; i < row.colCount(); ++i) {
                    // 获取列值，这里假设都是字符串类型，根据实际情况调整
                    record.push_back(row[i].get<float>());
                }
                data.push_back(record);
            }

            std::cout << "Retrieved " << data.size() << " rows from 'a09_4day_realtime'." << std::endl;

            // 可选：打印前几行数据
            for (size_t r = 0; r < std::min(data.size(), static_cast<size_t>(5)); ++r) { // Print first 5 rows
                for (size_t c = 0; c < data[r].size(); ++c) {
                    std::cout << data[r][c];
                    if (c < data[r].size() - 1) std::cout << "\t"; // Tab-separated
                }
                std::cout << std::endl;
            }


        }
        catch (const mysqlx::abi2::r0::Error& err) {
            std::cerr << "MySQL Error: " << err.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Standard Exception: " << e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "Unknown error occurred during MySQL operation." << std::endl;
        }
    }
};