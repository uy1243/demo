#pragma once
#include "ThostFtdcMdApi.h"
#include "Logger.h"

class CMdSpi : public CThostFtdcMdSpi {
private:
    CThostFtdcMdApi* m_api;
    const char* m_broker;
    const char* m_user;
    const char* m_pass;

public:
    CMdSpi(const char* broker, const char* user, const char* pass);
    void connect(const char* addr);
    void subscribe(const char** instrs, int count);

    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected(int nReason);
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField*, CThostFtdcRspInfoField*, int, bool);
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField*, CThostFtdcRspInfoField*, int, bool);
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField*);
};