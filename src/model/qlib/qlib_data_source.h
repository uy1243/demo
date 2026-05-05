// data_source/qlib_data_source.h
#ifndef QLIB_DATA_SOURCE_H
#define QLIB_DATA_SOURCE_H

#include "idata_source.h" // 假设这是你原来的基类
#include "../market_types.h" // 包含 TickData, BarData 等定义
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// 存储从 Qlib CSV 读取的数据结构
struct QlibFactorRecord {
    std::string instrument;
    std::string datetime; // 或者使用 chrono::system_clock::time_point
    double alpha001 = 0.0;
    double volatility_5d = 0.0;
    std::string signal = "HOLD"; // BUY, SELL, HOLD
    // ... 添加更多因子 ...
};

class QlibDataSource : public IDataSource {
public:
    // 构造函数接受 Qlib 生成的 CSV 文件路径
    explicit QlibDataSource(const std::string& csv_file_path);

    // 实现基类接口
    void connect() override;
    void disconnect() override;
    bool isConnected() const override;
    std::string getName() const override;

    // 获取 Qlib 计算的因子数据
    // 注意：这不是历史K线数据，而是预计算好的因子/信号
    std::vector<QlibFactorRecord> getFactorData(const std::string& instrument, const std::string& start_date, const std::string& end_date) const;

    // 可选：如果 Qlib 也提供了预处理的 Tick/Bar 数据，可以实现以下方法
    // std::vector<TickData> fetchTickData(const std::string& instrument, const std::string& start_date, const std::string& end_date) override;
    // std::vector<BarData> fetchBarData(const std::string& instrument, const std::string& start_date, const std::string& end_date) override;
    // 但通常，Qlib 的强项是因子计算和模型预测，而不是原始行情数据的存储

private:
    std::string csv_file_path_;
    bool connected_ = false;
    std::vector<QlibFactorRecord> factor_data_cache_; // 简单缓存

    // 内部辅助函数：解析 CSV
    void loadFactorDataFromCsv();
};

#endif // QLIB_DATA_SOURCE_H