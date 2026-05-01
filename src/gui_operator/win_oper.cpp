#include "win_oper.hpp"

void auto_login() {
    // 1. 找到登录窗口（改成你软件的窗口标题）
    HWND login_win = Win32Auto::find_window(L"SimNow 模拟交易客户端");

    // 2. 找输入框（一般有2个：账号、密码）
    HWND edit1 = Win32Auto::find_edit(login_win); // 账号输入框
    HWND edit2 = FindWindowExW(login_win, edit1, L"Edit", NULL); // 密码框

    // 3. 输入账号密码
    Win32Auto::set_text(edit1, L"123456");
    Win32Auto::set_text(edit2, L"123456");

    // 4. 找到【登录】按钮并点击
    HWND btn_login = Win32Auto::find_button(login_win, L"登录");
    Win32Auto::click(btn_login);
}

void auto_trade() {
    // 找到交易窗口
    HWND trade_win = Win32Auto::find_window(L"交易");

    // 找到价格输入框
    HWND price_edit = FindWindowExW(trade_win, NULL, L"Edit", L"");

    // 输入价格
    Win32Auto::set_text(price_edit, L"3600");

    // 找到【买入】按钮 点击
    HWND btn_buy = Win32Auto::find_button(trade_win, L"买入");
    Win32Auto::click(btn_buy);
}

void simnow_auto_order() {
    HWND main = Win32Auto::find_window(L"SimNow 交易客户端");

    // 点击下单
    HWND btn_order = Win32Auto::find_button(main, L"下单");
    Win32Auto::click(btn_order);
    Sleep(500);

    // 输入价格
    HWND ed_price = Win32Auto::find_edit(main);
    Win32Auto::set_text(ed_price, L"3580");

    // 点击买入
    HWND buy = Win32Auto::find_button(main, L"买入");
    Win32Auto::click(buy);
}