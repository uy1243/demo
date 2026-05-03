#include "monitor.h"
#include "account.h"
#include "logger.h"

FundMonitor& FundMonitor::Instance() {
    static FundMonitor ins;
    return ins;
}

void FundMonitor::check() {
    auto f = Account::Instance().getFundInfo();
    if (f.available < 50000) {
        LOG_WARN("资金不足！可用:%.2f", f.available);
    }
    if (f.total_asset < 950000) {
        LOG_ERROR("资金回撤超5%！预警！");
    }
}