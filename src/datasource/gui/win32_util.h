// win32_util.h
#ifndef WIN32_UTIL_H
#define WIN32_UTIL_H

#include <windows.h>
#include <vector>
#include <string>
#include <gdiplus.h>
#include <shellapi.h> // Add Shell API for ShellExecuteW

/**
 * @brief Windows 平台下窗口操作、输入模拟、屏幕截图的工具类
 *
 * 该类封装了常见的 Win32 API 和 GDI/GDI+ 操作，提供便捷的接口。
 * 注意：图像识别功能受限于 GDI，无法实现复杂的模板匹配。
 */
class Win32Util {
public:
    // 结构体用于存储窗口信息
    struct WindowInfo {
        HWND hwnd;
        std::string title;
        std::string class_name;
    };

    // 构造与析构
    Win32Util();
    ~Win32Util();

    // 禁止拷贝和赋值，因为可能涉及句柄等资源
    Win32Util(const Win32Util&) = delete;
    Win32Util& operator=(const Win32Util&) = delete;

    // 窗口相关
    std::vector<WindowInfo> FindWindows(const std::string& title = "", const std::string& class_name = "");
    bool IsWindowVisible(HWND hwnd) const;
    bool GetWindowRect(HWND hwnd, RECT& out_rect) const;
    bool BringWindowToTop(HWND hwnd) const;

    // 新增：查找或启动程序
    HWND FindWindowOrStartApp(const std::string& title, const std::string& class_name, const std::string& app_path);

    // 菜单相关
    bool ClickMenuItem(HWND hwnd, const std::string& menu_text);
    bool ClickMenuByIndex(HWND hwnd, int item_index);

    // 输入模拟
    void MouseClick(int x, int y) const;
    void MouseRightClick(int x, int y) const;
    void SendKey(char key) const;
    void SendText(const std::string& text) const;

    // 截图相关 (使用 GDI+ 获取原始像素数据)
    bool CaptureScreenToBMPFile(const std::string& filename) const;
    bool CaptureWindowToBMPFile(HWND hwnd, const std::string& filename) const;
    bool CaptureRegionToBMPFile(int x, int y, int width, int height, const std::string& filename) const;

    // 获取原始像素数据 (BGR 格式, 24bpp)
    std::vector<BYTE> CaptureScreenRaw(int& width, int& height) const;
    std::vector<BYTE> CaptureWindowRaw(HWND hwnd, int& width, int& height) const;
    std::vector<BYTE> CaptureRegionRaw(int x, int y, int width, int height, int& width_out, int& height_out) const;

    // GDI 版本的简单图像比较 (非模板匹配)
    bool ComparePixelAt(HWND hwnd, int x, int y, COLORREF target_color, int tolerance = 0) const;

    // 辅助函数
    void SleepMs(int milliseconds) const;

private:
    // GDI+ 初始化令牌
    ULONG_PTR gdiplusToken;

    // 回调函数，用于 EnumWindows
    static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam);
    // 存储找到的窗口列表
    static std::vector<WindowInfo>* s_found_windows;
    static std::string s_target_title;
    static std::string s_target_class_name;

    // 内部辅助函数
    bool SaveHBITMAPToFile(HBITMAP hBitmap, const std::string& filename) const;
};

#endif // WIN32_UTIL_H