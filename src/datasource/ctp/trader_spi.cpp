#include "trader_spi.h"
#include <string.h>

CTraderSpi::CTraderSpi(const char* broker, const char* user, const char* pass)
    : m_broker(broker), m_user(user), m_pass(pass), m_order_ref(0) {
    m_api = CThostFtdcTraderApi::CreateFtdcTraderApi();
    m_api->RegisterSpi(this);
}

void CTraderSpi::connect(const char* addr) {
    m_api->RegisterFront((char*)addr);
    m_api->Init();
}

void CTraderSpi::OnFrontConnected() {
    LOG_INFO("交易前置连接成功");
    CThostFtdcReqUserLoginField req{};
    strcpy(req.BrokerID, m_broker);
    strcpy(req.UserID, m_user);
    strcpy(req.Password, m_pass);
    m_api->ReqUserLogin(&req, 1);
}

void CTraderSpi::OnFrontDisconnected(int nReason) {
    LOG_ERR("交易前置断开，原因:%d", nReason);
}

void CTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pLogin, CThostFtdcRspInfoField* pRspInfo, int nId, bool bLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        LOG_ERR("交易登录失败:%s", GbkToUtf8( pRspInfo->ErrorMsg));
        return;
    }
    LOG_INFO("交易登录成功");
    m_front_id = pLogin->FrontID;
    m_session_id = pLogin->SessionID;
}

void CTraderSpi::send_order(const char* instr, char direction, char offset, double price, int vol) {
    CThostFtdcInputOrderField ord{};
    strcpy(ord.InstrumentID, instr);
    strcpy(ord.BrokerID, m_broker);
    strcpy(ord.InvestorID, m_user);

    ord.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    ord.Direction = direction;
    ord.CombOffsetFlag[0] = offset;
    ord.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

    ord.LimitPrice = price;
    ord.VolumeTotalOriginal = vol;

    ord.TimeCondition = THOST_FTDC_TC_GFD;
    ord.VolumeCondition = THOST_FTDC_VC_AV;
    ord.ContingentCondition = THOST_FTDC_CC_Immediately;
    ord.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    ord.IsAutoSuspend = 0;

    //ord.FrontID = m_front_id;
    //ord.SessionID = m_session_id;
    sprintf(ord.OrderRef, "%d", m_order_ref);

    m_api->ReqOrderInsert(&ord, m_order_ref);
    m_order_ref++;
}

void CTraderSpi::cancel_order(CThostFtdcOrderField* order) {
}

void CTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        LOG_ERR("下单失败: %s", pRspInfo->ErrorMsg);
    }
}

void CTraderSpi::OnRtnOrder(CThostFtdcOrderField* order) {
    LOG_INFO("订单更新 %s | 状态:%c", order->InstrumentID, order->OrderStatus);
}

void CTraderSpi::OnRtnTrade(CThostFtdcTradeField* trade) {
    LOG_INFO("成交 %s 价格:%.2f 数量:%d", trade->InstrumentID, trade->Price, trade->Volume);
}

void CTraderSpi::query_position() {
    CThostFtdcQryInvestorPositionField qry{};
    strcpy(qry.BrokerID, m_broker);
    strcpy(qry.InvestorID, m_user);
    m_api->ReqQryInvestorPosition(&qry, 1);
}

void CTraderSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pos, CThostFtdcRspInfoField* rsp, int n, bool last) {
    if (!pos) return;
    LOG_INFO("持仓 %s 今仓:%d 昨仓:%d", pos->InstrumentID, pos->PositionDate, pos->Position);
}