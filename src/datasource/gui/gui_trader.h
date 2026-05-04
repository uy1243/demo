// trading/your_api_trader.h
#pragma once
#include "trader_interface.h"
#include "../events/event_system.h"
#include <string>
#include <mutex>
#include <thread>

// 假设你的 API 头文件和库
// #include "your_api_header.h"

class YourApiTrader : public ITrader {
public:
    YourApiTrader(const std::string& config_path); // 你的 API 通常需要配置文件或参数
    ~YourApiTrader();

    OrderResponse placeOrder(const OrderRequest& req) override;
    CancelResponse cancelOrder(const CancelRequest& req) override;
    void start() override;
    void stop() override;
    std::string getName() const override { return "YOUR_API"; }

    void initialize(EventSystem* event_system);

private:
    std::string config_path_;
    void* api_handle_ = nullptr; // 你的 API 句柄，类型根据实际情况定
    EventSystem* event_system_ = nullptr;
    std::mutex request_mutex_;
    // ... 你的 API 特定的状态管理 ...

    // 你的 API 回调处理函数
    static void onOrderResponse(void* context, const char* local_id, int status, int filled, double avg_price, const char* msg);
    static void onCancelResponse(void* context, const char* local_id, bool success, const char* msg);

    // 辅助函数
    OrderStatus convertApiStatus(int api_status); // 将你的 API 状态转换为通用状态
};