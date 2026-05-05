// utils/prediction_loader.cpp
#include "prediction_loader.h"

std::unordered_map<std::string, DailyPredictions> PredictionLoader::LoadPredictions(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[PredictionLoader] 无法打开预测文件: " << file_path << std::endl;
        return {};
    }

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    try {
        nlohmann::json j = nlohmann::json::parse(content);
        return ParseJson(j.dump()); // Convert back to string for ParseJson, though ParseJson could take json obj directly
    }
    catch (const std::exception& e) {
        std::cerr << "[PredictionLoader] 解析 JSON 文件失败: " << e.what() << std::endl;
        return {};
    }
}

PredictionRecord PredictionLoader::GetPrediction(const std::unordered_map<std::string, DailyPredictions>& predictions,
    const std::string& date, const std::string& instrument) {
    auto date_it = predictions.find(date);
    if (date_it != predictions.end()) {
        auto inst_it = date_it->second.instrument_predictions.find(instrument);
        if (inst_it != date_it->second.instrument_predictions.end()) {
            return inst_it->second;
        }
    }
    // Return default if not found
    return PredictionRecord{ 0.0, "HOLD" };
}

std::unordered_map<std::string, DailyPredictions> PredictionLoader::ParseJson(const std::string& json_content) {
    nlohmann::json j = nlohmann::json::parse(json_content);
    std::unordered_map<std::string, DailyPredictions> result;

    for (auto& [date_str, instruments_obj] : j.items()) {
        DailyPredictions daily_preds;
        for (auto& [inst_str, record_obj] : instruments_obj.items()) {
            PredictionRecord record;
            record.score = record_obj["score"].get<double>();
            record.signal = record_obj["signal"].get<std::string>();
            daily_preds.instrument_predictions[inst_str] = record;
        }
        result[date_str] = daily_preds;
    }
    return result;
}