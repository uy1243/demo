#include "win_oper.hpp"
#include <Windows.h>

void auto_login() {
    HWND login_win = Win32Auto::find_window(L"交易客户端");

    HWND edit1 = Win32Auto::find_edit(login_win);
    HWND edit2 = FindWindowExW(login_win, edit1, L"Edit", NULL);

    Win32Auto::set_text(edit1, L"123456");
    Win32Auto::set_text(edit2, L"123456");

    HWND btn_login = Win32Auto::find_button(login_win, L"登录");
    Win32Auto::click(btn_login);
}

void auto_trade() {
    HWND trade_win = Win32Auto::find_window(L"交易");
    HWND price_edit = FindWindowExW(trade_win, NULL, L"Edit", NULL);

    Win32Auto::set_text(price_edit, L"3600");

    HWND btn_buy = Win32Auto::find_button(trade_win, L"买入");
    Win32Auto::click(btn_buy);
}

void simnow_auto_order() {
    HWND main = Win32Auto::find_window(L"交易客户端");

    HWND btn_order = Win32Auto::find_button(main, L"下单");
    Win32Auto::click(btn_order);
    Sleep(500);

    HWND ed_price = Win32Auto::find_edit(main);
    Win32Auto::set_text(ed_price, L"3580");

    HWND buy = Win32Auto::find_button(main, L"买入");
    Win32Auto::click(buy);
}

int main() {
    simnow_auto_order();
    return 0;
}