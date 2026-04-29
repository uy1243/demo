#include "md_spi.h"

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

// ===================== 已实现 =====================
void CMdSpi::OnFrontConnected() {
    LOG_INFO("行情前置连接成功");
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
        LOG_ERR("行情登录失败:%s", GbkToUtf8(pRspInfo->ErrorMsg));
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

// ===================== 【补齐】你缺少的函数！解决 LNK2001 =====================
void CMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField* pInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        LOG_ERR("订阅行情失败: %s, 错误: %s", pInstrument->InstrumentID, pRspInfo->ErrorMsg);
        return;
    }
    if (pInstrument) {
        LOG_INFO("订阅行情成功: %s", pInstrument->InstrumentID);
    }
}

// ===================== 【补齐】剩下所有可能报错的空实现 =====================
void CMdSpi::OnHeartBeatWarning(int nTimeLapse) {
    // 空实现
}

void CMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    // 空实现
}

void CMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    // 空实现
}

void CMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    // 空实现
}

void CMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField* pSpecificInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    // 空实现
}

void CMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField* pForQuoteRsp) {
    // 空实现
}