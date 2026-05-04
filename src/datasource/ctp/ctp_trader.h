// trading/ctp_trader.h
#pragma once
#include "trader_interface.h"
#include "../events/event_system.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>

// 假设你已经包含了 CTP 的头文件
// #include "ThostFtdcTraderApiSSE.h" 
// #include "ThostFtdcUserApiStructSSE.h"

class CtpTraderSpi; // 前置声明

class CtpTrader : public ITrader {
public:
    CtpTrader(const std::string& front_addr, const std::string& broker_id, const std::string& investor_id, const std::string& password);
    ~CtpTrader();

    OrderResponse placeOrder(const OrderRequest& req) override;
    CancelResponse cancelOrder(const CancelRequest& req) override;
    void start() override;
    void stop() override;
    std::string getName() const override { return "CTP"; }

    void initialize(EventSystem* event_system);

private:
    std::string front_addr_;
    std::string broker_id_;
    std::string investor_id_;
    std::string password_;

    // CTP API 相关指针
    void* api_ = nullptr; // CThostFtdcTraderApi*
    CtpTraderSpi* spi_ = nullptr; // CThostFtdcTraderSpi*

    EventSystem* event_system_ = nullptr;
    std::mutex request_mutex_;
    int request_id_ = 0;
    int order_ref_ = 0; // 报单引用

    std::unordered_map<std::string, std::string> local_to_exchange_map_; // 本地ID -> 交易所ID
    std::unordered_map<std::string, std::string> exchange_to_local_map_; // 交易所ID -> 本地ID

    int getNextRequestId();
    int getNextOrderRef();
};

// SPI 类，处理 CTP 回报
class CtpTraderSpi /* : public CThostFtdcTraderSpi */ { // 注释掉继承，因为头文件不可用
public:
    CtpTraderSpi(CtpTrader* parent, EventSystem* event_system);

    // 这些是 CTP SPI 的虚函数，你需要在这里实现
    void OnFrontConnected() override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRtnOrder(CThostFtdcOrderField* pOrder) override;
    void OnRtnTrade(CThostFtdcTradeField* pTrade) override;

private:
    CtpTrader* parent_;
    EventSystem* event_system_;
};