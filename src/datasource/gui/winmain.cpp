#include "win32_util.h"
#include <iostream>
#include <thread>

// 在 main.cpp 中添加这个调试辅助函数
void DebugPrintButtonPositions(Win32Util& util, HWND parent_hwnd) {
    HWND current = NULL;
    std::cout << "--- 开始枚举 TXPButton ---" << std::endl;

    // 循环查找所有类名为 TXPButton 的子控件
    while ((current = FindWindowExA(parent_hwnd, current, "TXPButton", NULL)) != NULL) {
        RECT rect;
        // GetWindowRect 获取的是屏幕绝对坐标
        // 如果需要相对坐标，可以用 GetClientRect 或者计算差值
        if (GetWindowRect(current, &rect)) {
            // 计算相对于父窗口的坐标
            RECT parent_rect;
            GetWindowRect(parent_hwnd, &parent_rect);

            int relative_x = rect.left - parent_rect.left;
            int relative_y = rect.top - parent_rect.top;

            char title[256];
            GetWindowTextA(current, title, sizeof(title));

            std::cout << "发现按钮: 标题=\"" << title
                << "\", 相对坐标=(" << relative_x << ", " << relative_y << ")"
                << ", 大小=" << (rect.right - rect.left) << "x" << (rect.bottom - rect.top)
                << std::endl;
        }
    }
    std::cout << "--- 枚举结束 ---" << std::endl;
}

bool click_login()
{
    Win32Util util;

    // 1. 找到父窗口
    auto windows = util.FindWindows("连接至服务器", "TClientRight");
    if (windows.empty()) {
        std::cerr << "未找到窗口" << std::endl;
        return -1;
    }

    HWND login_hwnd = windows[0].hwnd;
    std::cout << "找到窗口句柄: " << login_hwnd << std::endl;
	DebugPrintButtonPositions(util, login_hwnd);
    // ==========================================
    // 重点：在这里填入你测得的“登录”按钮相对坐标
    // 假设 X=50, Y=200 (请根据实际情况修改)
    int btn_x = 566;
    int btn_y = 302;
    // ==========================================

    std::cout << "正在尝试点击坐标 (" << btn_x << ", " << btn_y << ")..." << std::endl;

    if (util.ClickAtCoordinate(login_hwnd, btn_x, btn_y)) {
        std::cout << ">>> 点击指令发送成功！" << std::endl;
    }
    else {
        std::cerr << ">>> 点击失败" << std::endl;
    }

    return 0;

}

bool close_systeminfo_win()
{
    Win32Util util;

    // 1. 等待并查找系统消息窗口
    // 注意：窗口标题可能是 "系统消息" 或者具体的提示内容，类名是 TSysMsgDlg
    // 如果标题不确定，可以将第一个参数留空，或者尝试模糊匹配（这里假设标题可能是 "系统消息"）
    std::string msg_title = "系统消息"; // 如果不确定标题，留空尝试匹配类名，或者填入实际看到的标题
    std::string msg_class = "TSysMsgDlg";

    std::cout << "等待系统消息窗口出现..." << std::endl;

    HWND msg_hwnd = nullptr;
    // 最多等待 10 秒
    for (int i = 0; i < 50; ++i) {
        auto windows = util.FindWindows(msg_title, msg_class);
        if (!windows.empty()) {
            msg_hwnd = windows[0].hwnd;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (!msg_hwnd) {
        std::cerr << "未找到 TSysMsgDlg 窗口，可能已自动关闭或标题/类名不匹配。" << std::endl;
        return -1;
    }

    std::cout << "找到系统消息窗口: " << msg_hwnd << std::endl;

    // 2. 探测窗口内的按钮
    // 尝试查找 TXPButton (自定义按钮) 和 TButton (标准按钮)
    // 请观察控制台输出，找到位于右下角的那个按钮的坐标
    util.EnumerateChildControls(msg_hwnd, "TXPButton");
    util.EnumerateChildControls(msg_hwnd, "TButton");

    // 3. 点击关闭按钮
    // 根据上面的输出，手动填入“关闭”按钮的坐标
    // 假设通过观察，发现最后一个 TXPButton 坐标是 (300, 150)，大小 80x30
    // 我们需要点击它的中心点：
    // 中心X = 300 + 40 = 340
    // 中心Y = 150 + 15 = 165

    // 请在这里填入你实际测得的坐标
    int close_btn_x = 340; // 示例坐标
    int close_btn_y = 165; // 示例坐标

    std::cout << "正在尝试点击关闭按钮..." << std::endl;
    util.ClickAtCoordinate(msg_hwnd, close_btn_x, close_btn_y);

    return 0;
}
int main() {
    //click_login();
    close_systeminfo_win();
    return 0;
    Win32Util util;

    // 示例：查找或启动记事本
    HWND notepad_hwnd = util.FindWindowOrStartApp(
        "博易大师",
        "TMFrame",
        "pobo.exe"
    );

    if (notepad_hwnd) {
        std::cout << "Found or started pobo." << std::endl;
		click_login();
       }
    else {
        std::cout << "Could not find or start pobo." << std::endl;
    }

    return 0;
}