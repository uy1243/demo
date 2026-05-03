#include "settlement.h"
#include "account.h"
#include "logger.h"

void Settlement::dailySettle() {
    LOG_INFO("=== 每日结算 ===");
    auto& acc = Account::Instance();
    acc.settle();
    LOG_INFO("结算完成");
}