#include "win32_util.h"
#include <iostream>
#include <thread>

int main() {
    Win32Util util;

    // 尝试查找记事本，如果找不到则启动它
    // 注意：这里的 app_path 应该是你的 notepad.exe 的完整路径
    // 例如: "C:\\Windows\\System32\\notepad.exe"
    HWND notepad_hwnd = util.FindWindowOrStartApp(
        "无标题 - 记事本", // 标题，可能因语言设置不同而变化
        "Notepad",       // 类名
        "C:\\Windows\\System32\\notepad.exe" // 程序路径
    );

    if (notepad_hwnd) {
        std::cout << "Successfully got or created Notepad window." << std::endl;

        // 现在可以对 notepad_hwnd 进行后续操作
        util.BringWindowToTop(notepad_hwnd);
        util.SleepMs(1000);

        // 模拟输入
        util.SendText("Hello from Win32Util with auto-start!");
        util.MouseClick(100, 100);

        // 截图
        if (util.CaptureWindowToBMPFile(notepad_hwnd, "notepad_with_auto_start_screenshot.bmp")) {
            std::cout << "Screenshot saved." << std::endl;
        }
        else {
            std::cout << "Failed to capture screenshot." << std::endl;
        }

    }
    else {
        std::cout << "Could not find or start the specified application." << std::endl;
    }

    return 0;
}