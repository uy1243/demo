#pragma once
#include "ThostFtdcTraderApi.h"
#include "Logger.h"

class CTraderSpi : public CThostFtdcTraderSpi {
private:
    CThostFtdcTraderApi* m_api;
    const char* m_broker;
    const char* m_user;
    const char* m_pass;
    int m_front_id;
    int m_session_id;
    int m_order_ref;

public:
    CTraderSpi(const char* broker, const char* user, const char* pass);
    void connect(const char* addr);
    void send_order(const char* instr, char direction, char offset, double price, int vol);
    void cancel_order(CThostFtdcOrderField* order);
    void query_position();

    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected(int nReason);
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField*, CThostFtdcRspInfoField*, int, bool);
    virtual void OnRspOrderInsert(CThostFtdcInputOrderField*, CThostFtdcRspInfoField*, int, bool);
    virtual void OnRtnOrder(CThostFtdcOrderField*);
    virtual void OnRtnTrade(CThostFtdcTradeField*);
    virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField*, CThostFtdcRspInfoField*, int, bool);
};