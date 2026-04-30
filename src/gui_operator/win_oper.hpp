#pragma once
#include <Windows.h>
#include <string>

class Win32Auto {
public:
    // 1. 通过窗口标题找主窗口
    static HWND find_window(const std::wstring& title) {
        return FindWindowW(NULL, title.c_str());
    }

    // 2. 查找子控件（按钮/输入框）
    static HWND find_child(HWND parent, const std::wstring& class_name, const std::wstring& caption) {
        return FindWindowExW(parent, NULL, class_name.c_str(), caption.c_str());
    }

    // 3. 查找 按钮 （通过文字）
    static HWND find_button(HWND parent, const std::wstring& btn_text) {
        return find_child(parent, L"Button", btn_text);
    }

    // 4. 查找 编辑框 (输入框)
    static HWND find_edit(HWND parent) {
        return find_child(parent, L"Edit", L"");
    }

    // 5. 输入文本到输入框
    static void set_text(HWND hwnd, const std::wstring& text) {
        SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)text.c_str());
    }

    // 6. 获取文本
    static std::wstring get_text(HWND hwnd) {
        wchar_t buf[256] = { 0 };
        SendMessageW(hwnd, WM_GETTEXT, 256, (LPARAM)buf);
        return buf;
    }

    // 7. 模拟点击按钮
    static void click(HWND btn) {
        SendMessageW(btn, BM_CLICK, 0, 0);
    }

    // 8. 后台点击（不显示窗口）
    static void post_click(HWND btn) {
        PostMessageW(btn, BM_CLICK, 0, 0);
    }
};