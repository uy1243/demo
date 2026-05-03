#include "kline.h"
#include <ctime>

KLineGenerator& KLineGenerator::Instance() {
    static KLineGenerator ins;
    return ins;
}

void KLineGenerator::tick(const ExchangeQuote& q) {
    time_t now = time(0);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&now));
    std::string key = q.instrument + "_" + buf;

    if (current_bar_.find(key) == current_bar_.end()) {
        KLine k;
        k.instrument = q.instrument;
        k.time = buf;
        k.open = q.last;
        k.high = q.last;
        k.low = q.last;
        k.close = q.last;
        k.volume = q.volume;
        current_bar_[key] = k;
    }
    else {
        auto& k = current_bar_[key];
        k.high = std::max(k.high, q.last);
        k.low = std::min(k.low, q.last);
        k.close = q.last;
        k.volume += q.volume;
    }
}

std::vector<KLine> KLineGenerator::get1MinKLine(const std::string& inst) {
    return {};
}