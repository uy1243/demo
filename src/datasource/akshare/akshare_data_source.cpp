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

    // ... (映射其他基础字段) ...
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

    // --- 新增：映射深度数据 ---
    if (doc.HasMember("bid1") && doc["bid1"].IsNumber()) q.bid1 = doc["bid1"].GetDouble();
    if (doc.HasMember("bid2") && doc["bid2"].IsNumber()) q.bid2 = doc["bid2"].GetDouble();
    if (doc.HasMember("bid3") && doc["bid3"].IsNumber()) q.bid3 = doc["bid3"].GetDouble();
    if (doc.HasMember("bid4") && doc["bid4"].IsNumber()) q.bid4 = doc["bid4"].GetDouble();
    if (doc.HasMember("bid5") && doc["bid5"].IsNumber()) q.bid5 = doc["bid5"].GetDouble();
    if (doc.HasMember("ask1") && doc["ask1"].IsNumber()) q.ask1 = doc["ask1"].GetDouble();
    if (doc.HasMember("ask2") && doc["ask2"].IsNumber()) q.ask2 = doc["ask2"].GetDouble();
    if (doc.HasMember("ask3") && doc["ask3"].IsNumber()) q.ask3 = doc["ask3"].GetDouble();
    if (doc.HasMember("ask4") && doc["ask4"].IsNumber()) q.ask4 = doc["ask4"].GetDouble();
    if (doc.HasMember("ask5") && doc["ask5"].IsNumber()) q.ask5 = doc["ask5"].GetDouble();

    if (doc.HasMember("bid_vol1") && doc["bid_vol1"].IsInt64()) q.bid_vol1 = doc["bid_vol1"].GetInt64();
    if (doc.HasMember("bid_vol2") && doc["bid_vol2"].IsInt64()) q.bid_vol2 = doc["bid_vol2"].GetInt64();
    if (doc.HasMember("bid_vol3") && doc["bid_vol3"].IsInt64()) q.bid_vol3 = doc["bid_vol3"].GetInt64();
    if (doc.HasMember("bid_vol4") && doc["bid_vol4"].IsInt64()) q.bid_vol4 = doc["bid_vol4"].GetInt64();
    if (doc.HasMember("bid_vol5") && doc["bid_vol5"].IsInt64()) q.bid_vol5 = doc["bid_vol5"].GetInt64();
    if (doc.HasMember("ask_vol1") && doc["ask_vol1"].IsInt64()) q.ask_vol1 = doc["ask_vol1"].GetInt64();
    if (doc.HasMember("ask_vol2") && doc["ask_vol2"].IsInt64()) q.ask_vol2 = doc["ask_vol2"].GetInt64();
    if (doc.HasMember("ask_vol3") && doc["ask_vol3"].IsInt64()) q.ask_vol3 = doc["ask_vol3"].GetInt64();
    if (doc.HasMember("ask_vol4") && doc["ask_vol4"].IsInt64()) q.ask_vol4 = doc["ask_vol4"].GetInt64();
    if (doc.HasMember("ask_vol5") && doc["ask_vol5"].IsInt64()) q.ask_vol5 = doc["ask_vol5"].GetInt64();

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