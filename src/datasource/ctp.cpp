#pragma once
#include "imarket_data_source.hpp"
#include "ThostFtdcMdApi.h"

class CtpMarket : public IMarketDataSource, public CThostFtdcMdSpi
{
public:
    CtpMarket(const char* broker, const char* user, const char* pass, const char* addr);
    ~CtpMarket();

    // 接口实现
    void start() override;
    void subscribe(const std::string& instrument) override;
    void set_callback(IMarketCallback* callback) override;

private:
    // CTP 回调
    void OnFrontConnected() override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField*, CThostFtdcRspInfoField*, int, bool) override;
    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField*) override;

private:
    CThostFtdcMdApi* m_api;
    IMarketCallback* m_callback = nullptr;
    std::string m_broker, m_user, m_pass, m_addr;
};