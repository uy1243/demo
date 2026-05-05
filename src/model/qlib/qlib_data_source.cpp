// data_source/qlib_data_source.cpp
#include "qlib_data_source.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

// 简单的 CSV 解析器 (实际项目中可以使用成熟的库如 csv-parser)
// 这里仅作演示，假设 CSV 格式固定且无特殊字符
void parse_csv_line(const std::string& line, std::vector<std::string>& fields) {
    std::stringstream ss(line);
    std::string field;
    fields.clear();
    while (std::getline(ss, field, ',')) {
        fields.push_back(field);
    }
}

QlibDataSource::QlibDataSource(const std::string& csv_file_path)
    : csv_file_path_(csv_file_path) {
}

void QlibDataSource::connect() {
    try {
        loadFactorDataFromCsv();
        connected_ = true;
        std::cout << "[QlibDataSource] 连接成功，加载了 " << factor_data_cache_.size() << " 条因子数据记录" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[QlibDataSource] 连接失败: " << e.what() << std::endl;
        connected_ = false;
    }
}

void QlibDataSource::disconnect() {
    connected_ = false;
    factor_data_cache_.clear();
    std::cout << "[QlibDataSource] 断开连接" << std::endl;
}

bool QlibDataSource::isConnected() const {
    return connected_;
}

std::string QlibDataSource::getName() const {
    return "QlibDataSource";
}

std::vector<QlibFactorRecord> QlibDataSource::getFactorData(const std::string& instrument, const std::string& start_date, const std::string& end_date) const {
    if (!connected_) {
        throw std::runtime_error("QlibDataSource is not connected.");
    }

    std::vector<QlibFactorRecord> filtered_data;
    for (const auto& record : factor_data_cache_) {
        // 简单过滤：匹配合约和日期范围
        if (record.instrument == instrument && record.datetime >= start_date && record.datetime <= end_date) {
            filtered_data.push_back(record);
        }
    }
    return filtered_data;
}

void QlibDataSource::loadFactorDataFromCsv() {
    std::ifstream file(csv_file_path_);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open Qlib CSV file: " + csv_file_path_);
    }

    std::string line;
    bool header = true;
    while (std::getline(file, line)) {
        if (header) {
            header = false; // 跳过标题行
            continue;
        }

        std::vector<std::string> fields;
        parse_csv_line(line, fields);

        if (fields.size() < 5) { // 至少需要 instrument, datetime, alpha001, volatility_5d, signal
            std::cerr << "[Warning] Invalid CSV line: " << line << std::endl;
            continue;
        }

        QlibFactorRecord record;
        record.instrument = fields[0];
        record.datetime = fields[1];
        try {
            record.alpha001 = std::stod(fields[2]);
            record.volatility_5d = std::stod(fields[3]);
        }
        catch (...) {
            std::cerr << "[Warning] Error parsing numeric values in line: " << line << std::endl;
            continue;
        }
        record.signal = fields[4]; // 假设信号在第5列

        // ... 解析更多因子 ...

        factor_data_cache_.push_back(record);
    }
    file.close();
}