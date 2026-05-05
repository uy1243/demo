// utils/prediction_loader.h
#ifndef PREDICTION_LOADER_H
#define PREDICTION_LOADER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp" // Assuming you have added nlohmann/json

// 结构体存储单个预测记录
struct PredictionRecord {
    double score;
    std::string signal; // "BUY", "SELL", "HOLD"
};

// 结构体存储某一天的所有预测
struct DailyPredictions {
    std::unordered_map<std::string, PredictionRecord> instrument_predictions; // key: instrument, value: record
};

class PredictionLoader {
public:
    /**
     * @brief 从 JSON 文件加载最新的预测数据
     * @param file_path JSON 文件路径
     * @return map of date -> (map of instrument -> PredictionRecord)
     */
    static std::unordered_map<std::string, DailyPredictions> LoadPredictions(const std::string& file_path);

    /**
     * @brief 获取指定日期和合约的预测记录
     * @param predictions 预测数据
     * @param date 日期字符串 (YYYY-MM-DD)
     * @param instrument 合约代码
     * @return 预测记录，如果找不到则返回默认值
     */
    static PredictionRecord GetPrediction(const std::unordered_map<std::string, DailyPredictions>& predictions,
        const std::string& date, const std::string& instrument);

private:
    // Helper function to parse JSON string to internal structure
    static std::unordered_map<std::string, DailyPredictions> ParseJson(const std::string& json_content);
};

#endif // PREDICTION_LOADER_H