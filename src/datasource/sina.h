#pragma once
#include "imarket_data_source.hpp"
#include <thread>

class SinaMarket : public IMarketDataSource
{
public:
    void start() override;
    void subscribe(const std::string& instrument) override;
    void set_callback(IMarketCallback* callback) override;

private:
    void run_loop();
    TickData fetch_sina_tick(const std::string& instrument);

private:
    IMarketCallback* m_callback = nullptr;
    std::string m_instrument;
    std::thread m_thread;
};