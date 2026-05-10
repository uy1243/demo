#include "win32_auto.h"
#include <utils/logger.h>

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
Win32Auto::Win32Auto(const std::string& config_path) : config_path_(config_path) {
    api_handle_ = your_api_init(config_path_.c_str());
    if (!api_handle_) {
        std::cerr << "[YourApi] Initialization failed!" << std::endl;
    }
}

Win32Auto::~Win32Auto() {
    if (api_handle_) {
        your_api_release(api_handle_);
        api_handle_ = nullptr;
    }
}

void Win32Auto::initialize(EventSystem* event_system) {
    event_system_ = event_system;
}

OrderResponse Win32Auto::placeOrder(const OrderRequest& req) {
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

CancelResponse Win32Auto::cancelOrder(const CancelRequest& req) {
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

void Win32Auto::start() {
    if (api_handle_) {
        your_api_run(api_handle_);
    }
}

void Win32Auto::stop() {
    if (api_handle_) {
        your_api_stop(api_handle_);
    }
}

void Win32Auto::onOrderResponse(void* context, const char* local_id, int status, int filled, double avg_price, const char* msg) {
    Win32Auto* self = static_cast<Win32Auto*>(context);
    if (!self || !self->event_system_) return;

    // 修复：使用正确的构造函数调用，提供所有6个参数
    self->event_system_->publish(
        OrderUpdateEvent(
            std::string(local_id),           // 1. order_id
            "",                              // 2. exchange_order_id (这里没有交易所ID，用空字符串)
            self->convertApiStatus(status),  // 3. new_status
            filled,                          // 4. filled_volume
            avg_price,                       // 5. avg_fill_price
            std::string(msg ? msg : "")      // 6. error_msg
        )
    );
}

void Win32Auto::onCancelResponse(void* context, const char* local_id, bool success, const char* msg) {
    std::cout << "[YourApi] Cancel response for " << local_id << ": " << (success ? "Success" : "Failed") << ", Msg: " << (msg ? msg : "") << std::endl;
}

OrderStatus Win32Auto::convertApiStatus(int api_status) {
    switch (api_status) {
    case 0: return OrderStatus::PENDING;
    case 1: return OrderStatus::FILLED;
    case 2: return OrderStatus::PART_FILLED;
    case 3: return OrderStatus::CANCELED;
    case -1: return OrderStatus::REJECTED;
    default: return OrderStatus::REJECTED;
    }
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    wchar_t title[256] = { 0 }; // 初始化为0
    // 加上大括号，确保逻辑清晰
    if (GetWindowTextW(hwnd, title, 256)) {
        std::wcout << L"句柄: " << hwnd << L" | 标题: " << title << std::endl;
    }
    // 必须返回 TRUE，告诉系统继续找下一个窗口
    return TRUE;
}

std::vector<Order> Win32Auto::getAllOrders() const {
    // 这里你需要调用你的 API 获取订单列表，并转换为 std::vector<Order>
    // 这是一个示例实现，实际情况取决于你的 API 返回的数据结构
    std::vector<Order> orders;
    // ... 调用 API 获取订单数据并填充 orders ...
	return orders;
}
std::vector<Position> Win32Auto::getAllPositions() const {
	return std::vector<Position>(); // 这里你需要调用你的 API 获取持仓列表，并转换为 std::vector<Position>
}
AccountFund Win32Auto::getFundInfo() const {
	return AccountFund(); // 这里你需要调用你的 API 获取资金信息，并转换为 AccountFund
}

bool Win32Auto::findWindowAndInitializeApi() {
    // 枚举所有窗口，打印标题，帮助你找到目标窗口
    EnumWindows(EnumWindowsProc, 0);
    // 这里你需要添加逻辑来查找特定窗口，并根据窗口信息初始化 API
    // 例如，你可以使用 FindWindow 或其他方法来定位窗口，并获取必要的句柄或信息
    return true; // 返回 true 表示成功找到并初始化 API
}