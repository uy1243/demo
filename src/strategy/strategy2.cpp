#include "factor_strategy.h"
#include "../common/logger.h"
#include <cmath>

double ma(const std::vector<double>& v, int n) {
    if (v.size() < n) return 0;
    double s = 0;
    for (int i = 0;i < n;i++) s += v[v.size() - 1 - i];
    return s / n;
}

double rsi(const std::vector<double>& v, int n) {
    if (v.size() < n + 1) return 0;
    double up = 0, dn = 0;
    for (int i = 1;i <= n;i++) {
        double d = v[v.size() - i] - v[v.size() - i - 1];
        if (d > 0) up += d; else dn -= d;
    }
    if (dn == 0) return 100;
    double rs = up / dn;
    return 100 - 100 / (1 + rs);
}

FactorSignal FactorStrategy::run(const std::vector<KLine>& klines) {
    FactorSignal sig;
    if (klines.empty()) return sig;
    sig.inst = klines.back().inst;
    std::vector<double> closes;
    for (auto& k : klines) closes.push_back(k.close);
    double ma5 = ma(closes, 5);
    double ma20 = ma(closes, 20);
    double rsi14 = rsi(closes, 14);
    if (ma5 > ma20 && rsi14 < 70) sig.buy = 1;
    if (ma5 < ma20 && rsi14>30) sig.sell = 1;
    return sig;
}