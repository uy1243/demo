#include "akshare_data_source.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream> // For string stream operations
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

namespace {
    std::string exec_command(const std::string& cmd) {
        std::string result;
        FILE* pipe = _popen(cmd.c_str(), "r");
        if (!pipe) return "";

        char buffer;
        while (fgets(&buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        _pclose(pipe);
        return result;
    }
}

// --- 修改 fetchQuotes ---
std::vector<TickData> AkshareDataSource::fetchQuotes(const std::string& commodity) {
    std::vector<TickData> res;
    std::string command = "python akshare_worker.py quote " + commodity;
    std::string json_resp = exec_command(command);

    if (json_resp.empty()) {
        std::cerr << "DCE: Python script execution failed or returned empty." << std::endl;
        return res;
    }

    Document doc;
    doc.Parse(json_resp.c_str());
    if (doc.HasMember("error")) {
        std::cerr << "DCE Error from Python: " << doc["error"].GetString() << std::endl;
        return res;
    }

    TickData q{};
    q.source = getName();
    // ... (映射逻辑同上) ...
    if (doc.HasMember("instrument") && doc["instrument"].IsString())
        q.instrument = doc["instrument"].GetString();
    if (doc.HasMember("last") && doc["last"].IsNumber())
        q.last = doc["last"].GetDouble();
    if (doc.HasMember("open") && doc["open"].IsNumber())
        q.open = doc["open"].GetDouble();
    if (doc.HasMember("high") && doc["high"].IsNumber())
        q.high = doc["high"].GetDouble();
    if (doc.HasMember("low") && doc["low"].IsNumber())
        q.low = doc["low"].GetDouble();
    if (doc.HasMember("volume") && doc["volume"].IsInt64())
        q.volume = static_cast<long long>(doc["volume"].GetInt64());
    if (doc.HasMember("open_interest") && doc["open_interest"].IsNumber())
        q.open_interest = static_cast<long long>(doc["open_interest"].GetDouble());

    res.push_back(q);
    return res;
}

// --- 新的 fetchHistoricalData ---
std::multimap<std::string, TickData> AkshareDataSource::fetchHistoricalData(
    const std::string& instrument,
    const std::string& start_time,
    const std::string& end_time) {

    std::multimap<std::string, TickData> res;

    // 构造命令：调用 Python 脚本并传入 action, instrument, start_time, end_time
    std::string command = "python akshare_worker.py hist " + instrument + " " + start_time + " " + end_time;
    std::string json_resp = exec_command(command);

    if (json_resp.empty()) {
        std::cerr << "DCE: Historical data Python script execution failed or returned empty." << std::endl;
        return res;
    }

    Document doc;
    doc.Parse(json_resp.c_str());

    if (doc.HasMember("error")) {
        std::cerr << "DCE Historical Data Error from Python: " << doc["error"].GetString() << std::endl;
        return res;
    }

    // 检查是否存在 "ticks" 数组
    if (!doc.HasMember("ticks") || !doc["ticks"].IsArray()) {
        std::cerr << "DCE: Unexpected response format for historical data. Missing 'ticks' array." << std::endl;
        return res;
    }

    const Value& ticks_array = doc["ticks"];
    for (SizeType i = 0; i < ticks_array.Size(); ++i) {
        const Value& tick_obj = ticks_array[i];

        TickData t{};
        t.source = getName(); // 标记来源

        // 映射历史数据的字段
        if (tick_obj.HasMember("time") && tick_obj["time"].IsString()) {
            t.time = tick_obj["time"].GetString();
        }
        if (tick_obj.HasMember("instrument") && tick_obj["instrument"].IsString()) {
            t.instrument = tick_obj["instrument"].GetString();
        }
        if (tick_obj.HasMember("open") && tick_obj["open"].IsNumber()) {
            t.open = tick_obj["open"].GetDouble();
        }
        if (tick_obj.HasMember("high") && tick_obj["high"].IsNumber()) {
            t.high = tick_obj["high"].GetDouble();
        }
        if (tick_obj.HasMember("low") && tick_obj["low"].IsNumber()) {
            t.low = tick_obj["low"].GetDouble();
        }
        if (tick_obj.HasMember("last") && tick_obj["last"].IsNumber()) {
            // 使用 close 作为 last
            t.last = tick_obj["last"].GetDouble();
        }
        if (tick_obj.HasMember("volume") && tick_obj["volume"].IsInt64()) {
            t.volume = static_cast<long long>(tick_obj["volume"].GetInt64());
        }
        if (tick_obj.HasMember("open_interest") && tick_obj["open_interest"].IsNumber()) {
            t.open_interest = static_cast<long long>(tick_obj["open_interest"].GetDouble());
        }

        // 插入 multimap，key 是时间戳，value 是 TickData
        res.insert({ t.time, t });
    }

    return res;
}