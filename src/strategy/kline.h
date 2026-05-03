#pragma once
#include "market_data.h"
#include <map>

struct KLine {
    std::string instrument;
    std::string time;
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int volume = 0;
};

class KLineGenerator {
public:
    static KLineGenerator& Instance();
    void tick(const ExchangeQuote& q);
    std::vector<KLine> get1MinKLine(const std::string& inst);
private:
    KLineGenerator() = default;
    std::map<std::string, KLine> current_bar_;
    std::map<std::string, std::vector<KLine>> kline_1min_;
};