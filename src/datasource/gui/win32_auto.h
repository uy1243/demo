#pragma once
#include <Windows.h>
#include <string>
#include "../trader_interface.h"
#include "../../strategy/event_system.h"
#include <iostream>

class Win32Auto : public ITrader {
public:
    Win32Auto(const std::string& config_path); // 你的 API 通常需要配置文件或参数
    ~Win32Auto();
    OrderResponse placeOrder(const OrderRequest& req) override;
    CancelResponse cancelOrder(const CancelRequest& req) override;
    void start() override;
    void stop() override;
    std::string getName() const override { return "YOUR_API"; }

    void initialize(EventSystem* event_system);

    // 1. 通过标题查找窗口
    static HWND find_window(const std::wstring& title);

    // 2. 等待窗口出现（带超时）
    static HWND wait_window(const std::wstring& title, int timeout_ms = 20000);

    // 3. 查找子控件
    static HWND find_child(HWND parent, const std::wstring& class_name, const std::wstring& caption = L"");

    // 4. 查找普通按钮
    static HWND find_button(HWND parent, const std::wstring& btn_text);

    // 5. 查找 TXPButton（按文字）
    static HWND find_txbutton(HWND parent, const std::wstring& btn_text);

    // 6. 查找第N个 TXPButton
    static HWND find_txbutton_by_index(HWND parent, int index);

    // 7. 找最下方的 TXPButton（关闭/登录按钮专用）
    static HWND find_bottom_txbutton(HWND parent);

    // 8. 查找输入框
    static HWND find_edit(HWND parent, int index = 0);

    // 9. 设置文本
    static void set_text(HWND hwnd, const std::wstring& text);

    // 10. 获取文本
    static std::wstring get_text(HWND hwnd);

    // 11. 点击按钮（稳定）
    static void click(HWND btn);

    // 12. 获取控件相对父窗口的坐标
    static POINT get_control_pos(HWND parent, HWND control);

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