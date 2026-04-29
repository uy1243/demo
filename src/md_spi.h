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

    // ===================== 回调函数（全部声明，不缺一个） =====================
    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected(int nReason);
    virtual void OnHeartBeatWarning(int nTimeLapse);

    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField* pLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pData);
    virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp);
};