#pragma once
#include <Windows.h>
#include <string>

class Win32Auto {
public:
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
	void get_settlement_info();
	void insert_order(const std::string& inst, char dir, char offset, double price, int vol);
};