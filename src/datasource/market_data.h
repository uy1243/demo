#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <ctime>
#include <random>

// 行情结构体（期货 + 期权）
struct ExchangeQuote {
    std::string instrument;    // 合约代码 如 m2505
    std::string time;
    double last = 0;          // 最新价
    double open = 0;
    double high = 0;
    double low = 0;
    double bid1 = 0;          // 买一
    double ask1 = 0;          // 卖一
    int    volume = 0;        // 成交量
    double oi = 0;            // 持仓量
    bool is_option = false;   // 是否期权
};

// 全局缓存类型
using MarketCache = std::unordered_map<std::string, ExchangeQuote>;

// 行情缓存管理器 单例
class MarketManager
{
public:
    // 获取唯一实例
    static MarketManager& Instance();

    // 更新一条行情到缓存
    void UpdateQuote(const ExchangeQuote& q);

    // 根据合约代码获取行情
    bool GetQuote(const std::string& inst, ExchangeQuote& out_q);

    // 获取全市场所有行情
    MarketCache GetAllCache();

private:
    MarketManager() = default;
    ~MarketManager() = default;
    MarketManager(const MarketManager&) = delete;
    MarketManager& operator=(const MarketManager&) = delete;

    std::mutex mtx_;
    MarketCache cache_;
};

// 获取大商所行情 DCE
std::vector<ExchangeQuote> get_dce_quotes(const std::string& commodity);

// 获取郑商所行情 CZCE
std::vector<ExchangeQuote> get_czce_quotes(const std::string& commodity);

// 获取大商所行情 DCE
std::vector<ExchangeQuote> get_dce_quotes_random(const std::string& commodity);

// 获取郑商所行情 CZCE
std::vector<ExchangeQuote> get_czce_quotes_random(const std::string& commodity);

// 生成模拟行情并写入全局缓存
void GenerateRandomDceQuote();
void GenerateRandomCzceQuote();

void PrintMarketCache();