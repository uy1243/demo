#include "MdSpi.h"

CMdSpi::CMdSpi(const char* broker, const char* user, const char* pass)
    : m_broker(broker), m_user(user), m_pass(pass) {
    m_api = CThostFtdcMdApi::CreateFtdcMdApi();
    m_api->RegisterSpi(this);
}

void CMdSpi::connect(const char* addr) {
    m_api->RegisterFront((char*)addr);
    m_api->Init();
}

void CMdSpi::subscribe(const char** instrs, int count) {
    m_api->SubscribeMarketData((char**)instrs, count);
}

void CMdSpi::OnFrontConnected() {
    LOG_INFO("行情服务器连接成功");
    CThostFtdcReqUserLoginField req{};
    strcpy(req.BrokerID, m_broker);
    strcpy(req.UserID, m_user);
    strcpy(req.Password, m_pass);
    m_api->ReqUserLogin(&req, 1);
}

void CMdSpi::OnFrontDisconnected(int nReason) {
    LOG_ERR("行情断开，原因:%d", nReason);
}

void CMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pLogin, CThostFtdcRspInfoField* pRspInfo, int nId, bool bLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        LOG_ERR("行情登录失败:%s", pRspInfo->ErrorMsg);
        return;
    }
    LOG_INFO("行情登录成功");
}

void CMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField* pData) {
    LOG_INFO("%s | 最新价:%.2f | 买一:%.2f 卖一:%.2f | 成交量:%d",
        pData->InstrumentID,
        pData->LastPrice,
        pData->BidPrice1,
        pData->AskPrice1,
        pData->Volume);
}