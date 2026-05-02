#include "market_data.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;

#pragma comment(lib, "winhttp.lib")

using namespace std;

string http_get(const wstring& host, const wstring& path) {
    string resp;

    HINTERNET hSession = WinHttpOpen(L"client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, NULL, NULL, WINHTTP_FLAG_SECURE);

    BOOL bSend = WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);
    if (!bSend) {
        cout << "请求发送失败！" << endl;
    }

    WinHttpReceiveResponse(hRequest, NULL);

    char buf[4096];
    DWORD read = 0;
    while (WinHttpReadData(hRequest, buf, sizeof(buf), &read) && read > 0) {
        resp.append(buf, read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // 打印返回值（调试用）
    // cout << "响应：" << resp << endl;

    return resp;
}

// ==========================
// 大商所 DCE
// ==========================
vector<ExchangeQuote> get_dce_quotes(const string& commodity) {
    vector<ExchangeQuote> res;

    wstring host = L"www.dce.com.cn";
    wstring path = L"/portal/category/market/quote.do?code=" + wstring(commodity.begin(), commodity.end());

    string resp = http_get(host, path);
    if (resp.empty()) return res;

    Document doc;
    doc.Parse(resp.c_str());

    if (!doc.IsObject() || !doc.HasMember("data") || !doc["data"].HasMember("tables"))
        return res;

    const Value& tables = doc["data"]["tables"];
    if (!tables.IsArray()) return res;

    for (SizeType i = 0; i < tables.Size(); i++) {
        const Value& item = tables[i];
        if (!item.IsObject()) continue;

        ExchangeQuote q{};

        if (item.HasMember("instrumentId") && item["instrumentId"].IsString())
            q.instrument = item["instrumentId"].GetString();

        if (item.HasMember("lastPrice") && item["lastPrice"].IsDouble())
            q.last = item["lastPrice"].GetDouble();

        if (item.HasMember("openPrice") && item["openPrice"].IsDouble())
            q.open = item["openPrice"].GetDouble();

        if (item.HasMember("highPrice") && item["highPrice"].IsDouble())
            q.high = item["highPrice"].GetDouble();

        if (item.HasMember("lowPrice") && item["lowPrice"].IsDouble())
            q.low = item["lowPrice"].GetDouble();

        if (item.HasMember("bidPrice1") && item["bidPrice1"].IsDouble())
            q.bid1 = item["bidPrice1"].GetDouble();

        if (item.HasMember("askPrice1") && item["askPrice1"].IsDouble())
            q.ask1 = item["askPrice1"].GetDouble();

        if (item.HasMember("volume") && item["volume"].IsInt64())
            q.volume = (int)item["volume"].GetInt64();

        if (item.HasMember("openInterest") && item["openInterest"].IsDouble())
            q.oi = item["openInterest"].GetDouble();

        if (item.HasMember("updateTime") && item["updateTime"].IsString())
            q.time = item["updateTime"].GetString();

        res.push_back(q);
    }

    return res;
}

// ==========================
// 郑商所 CZCE（修复BUG）
// ==========================
vector<ExchangeQuote> get_czce_quotes(const string& commodity) {
    vector<ExchangeQuote> res;

    // 🔥 修复：原来的 host = L":" 是错的！
    wstring host = L"www.czce.com.cn";
    wstring path = L"/api/quotes/contract?contract=" + wstring(commodity.begin(), commodity.end());

    string resp = http_get(host, path);
    if (resp.empty()) return res;

    Document doc;
    doc.Parse(resp.c_str());

    if (!doc.IsObject() || !doc.HasMember("data") || !doc["data"].IsArray())
        return res;

    const Value& dataArray = doc["data"];
    for (SizeType i = 0; i < dataArray.Size(); i++) {
        const Value& item = dataArray[i];
        if (!item.IsObject()) continue;

        ExchangeQuote q{};

        if (item.HasMember("instrumentId") && item["instrumentId"].IsString())
            q.instrument = item["instrumentId"].GetString();

        if (item.HasMember("last") && item["last"].IsDouble())
            q.last = item["last"].GetDouble();

        if (item.HasMember("open") && item["open"].IsDouble())
            q.open = item["open"].GetDouble();

        if (item.HasMember("high") && item["high"].IsDouble())
            q.high = item["high"].GetDouble();

        if (item.HasMember("low") && item["low"].IsDouble())
            q.low = item["low"].GetDouble();

        if (item.HasMember("bid1") && item["bid1"].IsDouble())
            q.bid1 = item["bid1"].GetDouble();

        if (item.HasMember("ask1") && item["ask1"].IsDouble())
            q.ask1 = item["ask1"].GetDouble();

        if (item.HasMember("volume") && item["volume"].IsInt64())
            q.volume = (int)item["volume"].GetInt64();

        if (item.HasMember("oi") && item["oi"].IsDouble())
            q.oi = item["oi"].GetDouble();

        if (item.HasMember("time") && item["time"].IsString())
            q.time = item["time"].GetString();

        res.push_back(q);
    }

    return res;
}


// 获取当前系统时间 yyyy-MM-dd HH:mm:ss
static std::string get_current_time_str()
{
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

// 生成 [min,max] 随机浮点数
static double rand_double(double min, double max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(min, max);
    return dis(gen);
}

// 生成 [min,max] 随机整数
static int rand_int(int min, int max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

std::vector<ExchangeQuote> get_dce_quotes_random(const std::string& commodity) {
    std::vector<ExchangeQuote> res;

    // 基础基准价格
    double base1 = 3450.0;
    double base2 = 3480.0;

    // 构造两条合约模拟数据
    for (int idx = 0; idx < 2; ++idx)
    {
        ExchangeQuote q{};

        if (idx == 0)
            q.instrument = "m2509";
        else
            q.instrument = "m2511";

        double base = (idx == 0) ? base1 : base2;

        // 随机价格波动 ±15
        q.last = base + rand_double(-15.0, 15.0);
        q.open = q.last + rand_double(-8.0, 8.0);
        q.high = q.last + rand_double(2.0, 20.0);
        q.low = q.last - rand_double(2.0, 20.0);
        q.bid1 = q.last - 0.5;
        q.ask1 = q.last + 0.5;

        // 随机成交量、持仓量
        q.volume = rand_int(80000, 130000);
        q.oi = rand_int(400000, 600000);

        // 取当前系统时间
        q.time = get_current_time_str();

        res.push_back(q);
    }

    return res;
}

std::vector<ExchangeQuote> get_czce_quotes_random(const std::string& commodity) {
    std::vector<ExchangeQuote> res;

    double base1 = 6510.0;
    double base2 = 6540.0;

    for (int idx = 0; idx < 2; ++idx)
    {
        ExchangeQuote q{};

        if (idx == 0)
            q.instrument = "SR2509";
        else
            q.instrument = "SR2511";

        double base = (idx == 0) ? base1 : base2;

        q.last = base + rand_double(-20.0, 20.0);
        q.open = q.last + rand_double(-10.0, 10.0);
        q.high = q.last + rand_double(3.0, 25.0);
        q.low = q.last - rand_double(3.0, 25.0);
        q.bid1 = q.last - 0.5;
        q.ask1 = q.last + 0.5;

        q.volume = rand_int(70000, 95000);
        q.oi = rand_int(280000, 350000);

        q.time = get_current_time_str();

        res.push_back(q);
    }

    return res;
}

MarketManager& MarketManager::Instance()
{
    static MarketManager ins;
    return ins;
}

void MarketManager::UpdateQuote(const ExchangeQuote& q)
{
    std::lock_guard<std::mutex> lock(mtx_);
    cache_[q.instrument] = q;
}

bool MarketManager::GetQuote(const std::string& inst, ExchangeQuote& out_q)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = cache_.find(inst);
    if (it == cache_.end())
        return false;
    out_q = it->second;
    return true;
}

MarketCache MarketManager::GetAllCache()
{
    std::lock_guard<std::mutex> lock(mtx_);
    return cache_;
}

// ===================== 生成随机行情并写入缓存 =====================
void GenerateRandomDceQuote()
{
    // 豆粕两个合约
    std::vector<std::string> insts = { "m2509", "m2511" };
    std::vector<double> bases = { 3450.0, 3480.0 };

    for (size_t i = 0; i < insts.size(); ++i)
    {
        ExchangeQuote q;
        q.instrument = insts[i];
        q.last = bases[i] + rand_double(-15.0, 15.0);
        q.open = q.last + rand_double(-8.0, 8.0);
        q.high = q.last + rand_double(2.0, 20.0);
        q.low = q.last - rand_double(2.0, 20.0);
        q.bid1 = q.last - 0.5;
        q.ask1 = q.last + 0.5;
        q.volume = rand_int(80000, 130000);
        q.oi = rand_int(400000, 600000);
        q.time = get_current_time_str();

        // 写入全局缓存
        MarketManager::Instance().UpdateQuote(q);
    }
}

void GenerateRandomCzceQuote()
{
    // 白糖两个合约
    std::vector<std::string> insts = { "SR2509", "SR2511" };
    std::vector<double> bases = { 6510.0, 6540.0 };

    for (size_t i = 0; i < insts.size(); ++i)
    {
        ExchangeQuote q;
        q.instrument = insts[i];
        q.last = bases[i] + rand_double(-20.0, 20.0);
        q.open = q.last + rand_double(-10.0, 10.0);
        q.high = q.last + rand_double(3.0, 25.0);
        q.low = q.last - rand_double(3.0, 25.0);
        q.bid1 = q.last - 0.5;
        q.ask1 = q.last + 0.5;
        q.volume = rand_int(70000, 95000);
        q.oi = rand_int(280000, 350000);
        q.time = get_current_time_str();

        MarketManager::Instance().UpdateQuote(q);
    }
}

void PrintMarketCache()
{
    auto cache = MarketManager::Instance().GetAllCache();

    // 打印大商所
    std::cout << "=== 大商所 DCE 豆粕 （实时刷新）===" << std::endl;
    for (auto& p : cache) {
        auto& q = p.second;
        if (q.instrument.substr(0, 1) == "m") {
            printf("%-10s 最新:%.2f 买一:%.2f 卖一:%.2f 量:%d 时间:%s\n",
                q.instrument.c_str(), q.last, q.bid1, q.ask1, q.volume, q.time.c_str());
        }
    }

    // 打印郑商所
    std::cout << "\n=== 郑商所 CZCE 白糖 ===" << std::endl;
    for (auto& p : cache) {
        auto& q = p.second;
        if (q.instrument.substr(0, 2) == "SR") {
            printf("%-10s 最新:%.2f 买一:%.2f 卖一:%.2f 量:%d 时间:%s\n",
                q.instrument.c_str(), q.last, q.bid1, q.ask1, q.volume, q.time.c_str());
        }
    }
}