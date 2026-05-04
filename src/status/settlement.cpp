#include "settlement.h"
#include "account.h"
#include "utils/logger.h"

void Settlement::dailySettle() {
    LOG_INFO("=== 每日结算 ===");
    auto& acc = Account::Instance();
    LOG_INFO("结算完成");
}