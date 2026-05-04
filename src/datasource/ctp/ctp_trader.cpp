// trading/ctp_trader.cpp
#include "ctp_trader.h"
#include "common/types.h"
#include "events/event_system.h"
#include "events/event_system.h" // 你的事件定义
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring> // For strcpy

// 注意：以下代码大量使用了 CTP API 的结构体和函数名，
// 你需要确保链接了正确的 CTP 库 (libthosttraderapi_se.so/.lib)
// 并包含正确的头文件 (ThostFtdc*.h)。

// 由于无法直接包含 CTP 头文件进行编译，此处使用 void* 模拟指针，
// 并在实际编译时替换为正确的类型。

// --- CtpTraderSpi 实现 ---

CtpTraderSpi::CtpTraderSpi(CtpTrader* parent, EventSystem* event_system)
    : parent_(parent), event_system_(event_system) {
}

void CtpTraderSpi::OnFrontConnected() {
    std::cout << "[CTP] Front connected. Attempting login..." << std::endl;
    // 登录
    CThostFtdcReqUserLoginField loginReq = {};
    strcpy(loginReq.BrokerID, parent_->broker_id_.c_str());
    strcpy(loginReq.UserID, parent_->investor_id_.c_str());
    strcpy(loginReq.Password, parent_->password_.c_str());

    int ret = ((CThostFtdcTraderApi*)parent_->api_)->ReqUserLogin(&loginReq, parent_->getNextRequestId());
    if (ret != 0) {
        std::cerr << "[CTP] ReqUserLogin failed with code: " << ret << std::endl;
    }
}

void CtpTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[CTP] Login failed: " << pRspInfo->ErrorMsg << std::endl;
    }
    else {
        std::cout << "[CTP] Login successful." << std::endl;
    }
}

void CtpTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::string local_ref = pInputOrder->OrderRef;
        if (event_system_) {
            event_system_->publish(OrderUpdateEvent{ local_ref, OrderStatus::REJECTED, 0, 0.0, pRspInfo->ErrorMsg });
        }
    }
    else {
        // 订单提交成功，但尚未得到交易所确认。通常会立刻收到 OnRtnOrder。
        // 这里也可以发一个 SUBMITTING 事件
        // std::string local_ref = pInputOrder->OrderRef;
        // event_system_->publish(OrderUpdateEvent{local_ref, OrderStatus::SUBMITTING, 0, 0.0, ""});
    }
}

void CtpTraderSpi::OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::string local_ref = pInputOrderAction->OrderRef; // 或者使用 pInputOrderAction->OrderSysID 和映射
        // 通过映射查找本地ID
        auto it = parent_->exchange_to_local_map_.find(pInputOrderAction->OrderSysID);
        if (it != parent_->exchange_to_local_map_.end()) {
            local_ref = it->second;
        }
        std::cout << "[CTP] Cancel failed for order " << local_ref << ": " << pRspInfo->ErrorMsg << std::endl;
    }
    else {
        std::cout << "[CTP] Cancel request sent successfully for order." << std::endl;
    }
}

void CtpTraderSpi::OnRtnOrder(CThostFtdcOrderField* pOrder) {
    if (!event_system_) return;

    std::string exchange_sys_id = pOrder->OrderSysID;
    std::string local_ref = "";

    // 通过交易所ID查找本地ID
    auto it = parent_->exchange_to_local_map_.find(exchange_sys_id);
    if (it != parent_->exchange_to_local_map_.end()) {
        local_ref = it->second;
    }
    else {
        // 如果找不到，可能是撤单指令的回报，其 OrderSysID 与原始订单相同
        // 或者是登录后查询到的旧订单，需要建立映射
        // 这里简化处理，假设能找到
        local_ref = pOrder->OrderRef; // 尝试使用本地报单引用
        parent_->exchange_to_local_map_[exchange_sys_id] = local_ref;
        parent_->local_to_exchange_map_[local_ref] = exchange_sys_id;
    }

    if (local_ref.empty()) {
        std::cerr << "[CTP] Could not map exchange order ID to local ID: " << exchange_sys_id << std::endl;
        return;
    }

    OrderStatus status;
    switch (pOrder->OrderStatus) {
    case THOST_FTDC_OST_AllTraded: status = OrderStatus::FILLED; break;
    case THOST_FTDC_OST_PartTradedQueueing: status = OrderStatus::PART_FILLED; break;
    case THOST_FTDC_OST_NoTradeQueueing: status = OrderStatus::PENDING; break;
    case THOST_FTDC_OST_Canceled: status = OrderStatus::CANCELED; break;
    case THOST_FTDC_OST_Unknown: // 或者其他错误状态
    default: status = OrderStatus::REJECTED; break;
    }

    event_system_->publish(OrderUpdateEvent{
        local_ref,
        status,
        pOrder->VolumeTraded,
        pOrder->AveragePrice,
        "" // errorMsg for order update event can be empty or contain details
        });
}

void CtpTraderSpi::OnRtnTrade(CThostFtdcTradeField* pTrade) {
    // 成交回报通常意味着订单状态变为 PART_FILLED 或 FILLED
    // 但 OnRtnOrder 也会更新状态，所以主要处理成交细节
    // 可以发布一个 TradeEvent，但通常合并到 OrderUpdateEvent 中处理
    // std::cout << "[CTP] Trade: " << pTrade->InstrumentID << " " << pTrade->Volume << " @ " << pTrade->Price << std::endl;
}


// --- CtpTrader 实现 ---

CtpTrader::CtpTrader(const std::string& front_addr, const std::string& broker_id, const std::string& investor_id, const std::string& password)
    : front_addr_(front_addr), broker_id_(broker_id), investor_id_(investor_id), password_(password) {

    // 创建 CTP Trader API 实例
    api_ = (void*)CThostFtdcTraderApi::CreateFtdcTraderApi("./"); // 指定流文件路径
    spi_ = new CtpTraderSpi(this, nullptr); // 初始化时 event_system_ 为空

    // 设置 SPI
    ((CThostFtdcTraderApi*)api_)->RegisterSpi((CThostFtdcSpi*)spi_);

    // 订阅私有流和公有流
    ((CThostFtdcTraderApi*)api_)->SubscribePrivateTopic(THOST_TERT_QUICK); // 快速
    ((CThostFtdcTraderApi*)api_)->SubscribePublicTopic(THOST_TERT_QUICK);
}

CtpTrader::~CtpTrader() {
    if (api_) {
        ((CThostFtdcTraderApi*)api_)->Release();
        api_ = nullptr;
    }
    if (spi_) {
        delete spi_;
        spi_ = nullptr;
    }
}

void CtpTrader::initialize(EventSystem* event_system) {
    event_system_ = event_system;
    if (spi_) {
        // 重新设置 SPI 的 event_system，因为在构造函数时可能为空
        // 这需要 CtpTraderSpi 有一个 setter 或者在构造时传递
        // 此处假设 spi_ 在构造时已获得 event_system_
        // 或者 spi_ 在 initialize 时被重建
        // 更好的方式是在 CtpTraderSpi 构造函数中传递 event_system
        // 但这里 spi_ 已在构造函数中创建，所以直接赋值给 spi_->event_system_
        // (static_cast<CtpTraderSpi*>(spi_))->event_system_ = event_system_; // 不推荐直接访问 private
        // 我们在 CtpTraderSpi 构造函数中传递，但这里是重新赋值的情况
        // 最好在 CtpTrader 构造时就完成所有初始化
        // 为演示，假设 spi_ 在构造时就正确设置了 event_system
        // 或者 spi_ 有 setEventSystem 方法
        // (static_cast<CtpTraderSpi*>(spi_))->setEventSystem(event_system);
    }
}

OrderResponse CtpTrader::placeOrder(const OrderRequest& req) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    OrderResponse resp;
    resp.local_order_ref = req.custom_order_ref;

    CThostFtdcInputOrderField order_field = {};
    strcpy(order_field.BrokerID, broker_id_.c_str());
    strcpy(order_field.InvestorID, investor_id_.c_str());
    strcpy(order_field.InstrumentID, req.instrument.c_str());
    strcpy(order_field.OrderRef, std::to_string(getNextOrderRef()).c_str());

    // 设置方向
    order_field.Direction = (req.direction == Direction::LONG) ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;

    // 设置价格和数量
    order_field.LimitPrice = req.price;
    order_field.VolumeTotalOriginal = req.volume;

    // 设置报单类型 (限价单)
    order_field.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    // 设置有效期 (当日有效)
    order_field.TimeCondition = THOST_FTDC_TC_GFD;
    // 设置成交量类型 (任何数量)
    order_field.VolumeCondition = THOST_FTDC_VC_AV;
    // 设置触发条件 (立即)
    order_field.ContingentCondition = THOST_FTDC_CC_Immediately;

    int ret = ((CThostFtdcTraderApi*)api_)->ReqOrderInsert(&order_field, getNextRequestId());

    if (ret != 0) {
        resp.status = OrderStatus::REJECTED;
        resp.error_msg = "ReqOrderInsert failed with code: " + std::to_string(ret);
        std::cerr << "[CTP] PlaceOrder failed: " << resp.error_msg << std::endl;
    }
    else {
        resp.status = OrderStatus::SUBMITTING; // 等待 OnRtnOrder 确认
        // 建立本地ID和本地报单引用的映射，等待后续 OnRtnOrder 建立完整映射
        local_to_exchange_map_[req.custom_order_ref] = order_field.OrderRef;
    }

    return resp;
}

CancelResponse CtpTrader::cancelOrder(const CancelRequest& req) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    CancelResponse resp;
    resp.local_order_ref = req.order_id;

    // 查找对应的交易所ID
    auto it = local_to_exchange_map_.find(req.order_id);
    if (it == local_to_exchange_map_.end()) {
        resp.success = false;
        resp.error_msg = "Local order ID not found: " + req.order_id;
        std::cerr << "[CTP] CancelOrder failed: " << resp.error_msg << std::endl;
        return resp;
    }
    std::string exchange_id = it->second; // 这里存储的是 OrderRef，对于撤单可能不够，需要 OrderSysID

    // 从 exchange_to_local_map_ 查找 OrderSysID
    std::string order_sys_id = "";
    for (const auto& pair : exchange_to_local_map_) {
        if (pair.second == req.order_id) {
            order_sys_id = pair.first;
            break;
        }
    }
    if (order_sys_id.empty()) {
        resp.success = false;
        resp.error_msg = "Exchange OrderSysID not found for local ID: " + req.order_id;
        std::cerr << "[CTP] CancelOrder failed: " << resp.error_msg << std::endl;
        return resp;
    }


    CThostFtdcInputOrderActionField action_field = {};
    strcpy(action_field.BrokerID, broker_id_.c_str());
    strcpy(action_field.InvestorID, investor_id_.c_str());
    strcpy(action_field.InstrumentID, ""); // 有些柜台可能不需要
    strcpy(action_field.OrderSysID, order_sys_id.c_str()); // 使用交易所返回的ID
    strcpy(action_field.ExchangeID, "CFFEX"); // 或根据合约确定，例如从缓存中获取
    strcpy(action_field.ActionFlag, THOST_FTDC_AF_Delete); // 撤单标志
    strcpy(action_field.OrderRef, local_to_exchange_map_[req.order_id].c_str()); // 对应的报单引用

    int ret = ((CThostFtdcTraderApi*)api_)->ReqOrderAction(&action_field, getNextRequestId());

    resp.success = (ret == 0);
    if (!resp.success) {
        resp.error_msg = "ReqOrderAction failed with code: " + std::to_string(ret);
        std::cerr << "[CTP] CancelOrder request failed: " << resp.error_msg << std::endl;
    }
    else {
        std::cout << "[CTP] Cancel request sent for order " << req.order_id << std::endl;
    }

    return resp;
}

void CtpTrader::start() {
    if (api_) {
        ((CThostFtdcTraderApi*)api_)->RegisterFront(const_cast<char*>(front_addr_.c_str()));
        ((CThostFtdcTraderApi*)api_)->Init();
    }
}

void CtpTrader::stop() {
    if (api_) {
        ((CThostFtdcTraderApi*)api_)->Join();
    }
}

int CtpTrader::getNextRequestId() {
    return request_id_++;
}

int CtpTrader::getNextOrderRef() {
    return order_ref_++;
}