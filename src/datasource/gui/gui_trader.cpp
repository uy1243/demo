#include "gui_trader.h"
#include "common/types.h"
#include "../../strategy/event_system.h"
#include <iostream>
#include <mutex>

// ===================== 修复：实现空的 API 桩函数 =====================
extern "C" void* your_api_init(const char* config_path) {
    std::cout << "[YourApi] your_api_init 模拟初始化成功" << std::endl;
    return (void*)1; // 返回非空模拟有效句柄
}

extern "C" int your_api_place_order(void* handle, const char* instrument, char direction, double price, int volume, const char* local_ref) {
    std::cout << "[YourApi] 模拟下单: " << instrument << " " << (direction == 'B' ? "买" : "卖")
        << " 价格:" << price << " 手数:" << volume << std::endl;
    return 0; // 成功
}

extern "C" int your_api_cancel_order(void* handle, const char* local_ref) {
    std::cout << "[YourApi] 模拟撤单: " << local_ref << std::endl;
    return 0;
}

extern "C" void your_api_release(void* handle) {}
extern "C" void your_api_run(void* handle) {}
extern "C" void your_api_stop(void* handle) {}

// ===================== 以下是你原来的代码，完全不动 =====================
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

    char dir_char = (req.direction == Direction::LONG) ? 'B' : 'S';
    int ret_code = your_api_place_order(api_handle_, req.instrument.c_str(), dir_char, req.price, req.volume, req.custom_order_ref.c_str());

    if (ret_code != 0) {
        resp.status = OrderStatus::REJECTED;
        resp.error_msg = "API place order failed with code: " + std::to_string(ret_code);
    }
    else {
        resp.status = OrderStatus::SUBMITTING;
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
        your_api_run(api_handle_);
    }
}

void YourApiTrader::stop() {
    if (api_handle_) {
        your_api_stop(api_handle_);
    }
}

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
    std::cout << "[YourApi] Cancel response for " << local_id << ": " << (success ? "Success" : "Failed") << ", Msg: " << (msg ? msg : "") << std::endl;
}

OrderStatus YourApiTrader::convertApiStatus(int api_status) {
    switch (api_status) {
    case 0: return OrderStatus::PENDING;
    case 1: return OrderStatus::FILLED;
    case 2: return OrderStatus::PART_FILLED;
    case 3: return OrderStatus::CANCELED;
    case -1: return OrderStatus::REJECTED;
    default: return OrderStatus::REJECTED;
    }
}