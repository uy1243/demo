// trading/your_api_trader.cpp
#include "your_api_trader.h"
#include "common/types.h"
#include "events/event_system.h"
#include <iostream>
#include <cstring> // For strcpy if needed by your API

// 假设你的 API 初始化函数
extern "C" void* your_api_init(const char* config_path);
extern "C" int your_api_place_order(void* handle, const char* instrument, char direction, double price, int volume, const char* local_ref);
extern "C" int your_api_cancel_order(void* handle, const char* local_ref);
extern "C" void your_api_release(void* handle);
extern "C" void your_api_run(void* handle); // 启动事件循环
extern "C" void your_api_stop(void* handle); // 停止事件循环

YourApiTrader::YourApiTrader(const std::string& config_path) : config_path_(config_path) {
    api_handle_ = your_api_init(config_path_.c_str());
    if (!api_handle_) {
        std::cerr << "[YourApi] Initialization failed!" << std::endl;
    }
}

YourApiTrader::~YourApiTrader() {
    if (api_handle_) {
        your_api_release(api_handle_);
        api_handle_ = nullptr;
    }
}

void YourApiTrader::initialize(EventSystem* event_system) {
    event_system_ = event_system;
    // 如果你的 API 需要设置回调上下文
    // your_api_set_context(api_handle_, this);
    // your_api_set_callbacks(api_handle_, onOrderResponse, onCancelResponse, ...);
}

OrderResponse YourApiTrader::placeOrder(const OrderRequest& req) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    OrderResponse resp;
    resp.local_order_ref = req.custom_order_ref;

    if (!api_handle_) {
        resp.status = OrderStatus::REJECTED;
        resp.error_msg = "API handle is invalid.";
        return resp;
    }

    char dir_char = (req.direction == Direction::LONG) ? 'B' : 'S'; // 假设你的 API 用字符表示方向
    int ret_code = your_api_place_order(api_handle_, req.instrument.c_str(), dir_char, req.price, req.volume, req.custom_order_ref.c_str());

    if (ret_code != 0) { // 假设非0为错误
        resp.status = OrderStatus::REJECTED;
        resp.error_msg = "API place order failed with code: " + std::to_string(ret_code);
    }
    else {
        resp.status = OrderStatus::SUBMITTING; // 等待回报
    }

    return resp;
}

CancelResponse YourApiTrader::cancelOrder(const CancelRequest& req) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    CancelResponse resp;
    resp.local_order_ref = req.order_id;

    if (!api_handle_) {
        resp.success = false;
        resp.error_msg = "API handle is invalid.";
        return resp;
    }

    int ret_code = your_api_cancel_order(api_handle_, req.order_id.c_str());

    resp.success = (ret_code == 0);
    if (!resp.success) {
        resp.error_msg = "API cancel order failed with code: " + std::to_string(ret_code);
    }

    return resp;
}

void YourApiTrader::start() {
    if (api_handle_) {
        // 启动你的 API 的事件循环线程
        your_api_run(api_handle_);
    }
}

void YourApiTrader::stop() {
    if (api_handle_) {
        your_api_stop(api_handle_);
    }
}

// 回调函数实现
void YourApiTrader::onOrderResponse(void* context, const char* local_id, int status, int filled, double avg_price, const char* msg) {
    YourApiTrader* self = static_cast<YourApiTrader*>(context);
    if (!self || !self->event_system_) return;

    self->event_system_->publish(OrderUpdateEvent{
        std::string(local_id),
        self->convertApiStatus(status),
        filled,
        avg_price,
        std::string(msg ? msg : "")
        });
}

void YourApiTrader::onCancelResponse(void* context, const char* local_id, bool success, const char* msg) {
    // 可以发布一个单独的 CancelEvent，或者在 Account 中处理 CancelRequest 的响应
    std::cout << "[YourApi] Cancel response for " << local_id << ": " << (success ? "Success" : "Failed") << ", Msg: " << (msg ? msg : "") << std::endl;
}

OrderStatus YourApiTrader::convertApiStatus(int api_status) {
    // 根据你的 API 返回的状态码进行转换
    // 这里是示例
    switch (api_status) {
    case 0: return OrderStatus::PENDING; // 假设 0 是已报单
    case 1: return OrderStatus::FILLED;   // 假设 1 是全部成交
    case 2: return OrderStatus::PART_FILLED; // 假设 2 是部分成交
    case 3: return OrderStatus::CANCELED; // 假设 3 是已撤单
    case -1: return OrderStatus::REJECTED; // 假设 -1 是已拒单
    default: return OrderStatus::REJECTED;
    }
}