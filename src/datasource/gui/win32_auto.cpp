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

HWND Win32Auto::find_window(const std::wstring& title) {
    return FindWindowW(NULL, title.c_str());
}

HWND Win32Auto::wait_window(const std::wstring& title, int timeout_ms) {
    HWND hwnd = NULL;
    int waited = 0;
    while (waited < timeout_ms) {
        hwnd = find_window(title);
        if (hwnd) return hwnd;
        Sleep(500);
        waited += 500;
    }
    return NULL;
}

HWND Win32Auto::find_child(HWND parent, const std::wstring& class_name, const std::wstring& caption) {
    return FindWindowExW(parent, NULL, class_name.c_str(), caption.c_str());
}

HWND Win32Auto::find_button(HWND parent, const std::wstring& btn_text) {
    return find_child(parent, L"Button", btn_text);
}

HWND Win32Auto::find_txbutton(HWND parent, const std::wstring& btn_text) {
    HWND cur = NULL;
    while (true) {
        cur = FindWindowExW(parent, cur, L"TXPButton", NULL);
        if (!cur) break;
        if (get_text(cur) == btn_text) {
            return cur;
        }
    }
    return NULL;
}

HWND Win32Auto::find_txbutton_by_index(HWND parent, int index) {
    HWND cur = NULL;
    for (int i = 0; i <= index; ++i) {
        cur = FindWindowExW(parent, cur, L"TXPButton", NULL);
        if (!cur) break;
    }
    return cur;
}

POINT Win32Auto::get_control_pos(HWND parent, HWND control) {
    RECT rc;
    GetWindowRect(control, &rc);
    POINT pt = { rc.left, rc.top };
    ScreenToClient(parent, &pt);
    return pt;
}

HWND Win32Auto::find_bottom_txbutton(HWND parent) {
    HWND best = NULL;
    int max_y = -1;

    HWND cur = NULL;
    while (true) {
        cur = FindWindowExW(parent, cur, L"TXPButton", NULL);
        if (!cur) break;

        POINT pos = get_control_pos(parent, cur);
        if (pos.y > max_y) {
            max_y = pos.y;
            best = cur;
        }
    }
    return best;
}

HWND Win32Auto::find_edit(HWND parent, int index) {
    HWND cur = NULL;
    for (int i = 0; i <= index; ++i) {
        cur = FindWindowExW(parent, cur, L"Edit", NULL);
        if (!cur) break;
    }
    return cur;
}

void Win32Auto::set_text(HWND hwnd, const std::wstring& text) {
    SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)text.c_str());
}

std::wstring Win32Auto::get_text(HWND hwnd) {
    wchar_t buf[256] = { 0 };
    SendMessageW(hwnd, WM_GETTEXT, 256, (LPARAM)buf);
    return buf;
}

void Win32Auto::click(HWND btn) {
    if (!btn) return;
    SendMessageW(btn, BM_CLICK, 0, 0);
    Sleep(300);
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

int main()
{
    std::cout << "=== 开始枚举所有顶层窗口 ===" << std::endl;
    EnumWindows(EnumWindowsProc, 0);

    HWND hd = Win32Auto::find_window(L"Vector CANdb++ Editor - C:\Users\Administrator\Desktop\ADASDBC_receive.dbc - [Overall View]");
    if(!hd){
        std::cout << "未找到窗口" << std::endl;
	}
    else{
        std::cout << "找到窗口: " << hd << std::endl;
    }
    system("pause");

    return 0;
}