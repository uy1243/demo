#pragma once

class FundMonitor {
public:
    static FundMonitor& Instance();
    void check();
private:
    FundMonitor() = default;
};