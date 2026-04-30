#pragma once
#include "market_data.hpp"

// 行情回调接口
class IMarketCallback
{
public:
    virtual ~IMarketCallback() = default;
    virtual void on_tick(const TickData& tick) = 0;
};

// 统一数据源接口
class IMarketDataSource
{
public:
    virtual ~IMarketDataSource() = default;
    virtual void start() = 0;
    virtual void subscribe(const std::string& instrument) = 0;
    virtual void set_callback(IMarketCallback* callback) = 0;
};