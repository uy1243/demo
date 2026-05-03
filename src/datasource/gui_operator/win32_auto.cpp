#include "win32_auto.hpp"
#include <utils/logger.h>

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

void Win32Auto::get_settlement_info()
{

}

void  Win32Auto::insert_order(const std::string& inst, char dir, char offset, double price, int vol)
{

}
void auto_login() {
    HWND login_win = Win32Auto::find_window(L"连接至服务器");
    LOG_INFO("找到登录窗口: %p", login_win);

    // 找最下方的登录按钮（TXPButton）
    HWND btn_login = Win32Auto::find_bottom_txbutton(login_win);
    Win32Auto::click(btn_login);

    // 关闭系统消息
    HWND info_win = Win32Auto::wait_window(L"系统消息", 30000);
    if (info_win) {
        LOG_INFO("找到系统消息窗口: %p", info_win);
        HWND btn_close = Win32Auto::find_txbutton_by_index(info_win, 0);
        if (btn_close) {
            LOG_INFO("关闭按钮: %p", btn_close);
            Win32Auto::click(btn_close);
        }
    }
    Sleep(1000);
}

int main() {
    auto_login();
    return 0;
}