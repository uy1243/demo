// win32_util.cpp
#include "win32_util.h"
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <psapi.h> // For GetModuleFileNameExA, requires psapi.lib
#include <gdiplus.h>
#include <wingdi.h>
#include <shellapi.h> // Include Shell API
#include <fstream>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib") // Link shell32 for ShellExecuteW

// 静态成员定义
std::vector<Win32Util::WindowInfo>* Win32Util::s_found_windows = nullptr;
std::string Win32Util::s_target_title = "";
std::string Win32Util::s_target_class_name = "";

Win32Util::Win32Util() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

Win32Util::~Win32Util() {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

/**
 * @brief EnumWindows 的回调函数，用于收集符合条件的窗口。
 */
BOOL CALLBACK Win32Util::EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    int length = GetWindowTextLengthA(hwnd);
    std::vector<char> title(length + 1);
    GetWindowTextA(hwnd, title.data(), length + 1);

    char class_name[256];
    GetClassNameA(hwnd, class_name, sizeof(class_name));

    // 修正：使用 Windows API
    bool isVisible = ::IsWindowVisible(hwnd) != 0;

    bool match = true;
    if (!s_target_title.empty() && std::string(title.data()) != s_target_title) {
        match = false;
    }
    if (!s_target_class_name.empty() && std::string(class_name) != s_target_class_name) {
        match = false;
    }

    if (match && isVisible) {
        WindowInfo info;
        info.hwnd = hwnd;
        info.title = std::string(title.data());
        info.class_name = std::string(class_name);
        s_found_windows->push_back(info);
    }

    return TRUE;
}

// 窗口相关实现
std::vector<Win32Util::WindowInfo> Win32Util::FindWindows(const std::string& title, const std::string& class_name) {
    std::vector<WindowInfo> windows;
    s_found_windows = &windows;
    s_target_title = title;
    s_target_class_name = class_name;

    EnumWindows(EnumWindowsCallback, 0);

    s_found_windows = nullptr;
    return windows;
}

bool Win32Util::IsWindowVisible(HWND hwnd) const {
    return ::IsWindowVisible(hwnd) != 0;
}

bool Win32Util::GetWindowRect(HWND hwnd, RECT& out_rect) const {
    return ::GetWindowRect(hwnd, &out_rect) != 0;
}

bool Win32Util::BringWindowToTop(HWND hwnd) const {
    return SetForegroundWindow(hwnd) != 0;
}

// 新增：查找窗口，如果找不到则启动应用程序
HWND Win32Util::FindWindowOrStartApp(const std::string& title, const std::string& class_name, const std::string& app_path) {
    auto windows = FindWindows(title, class_name);
    if (!windows.empty()) {
        std::cout << "Found existing window: " << windows[0].title << std::endl;
        return windows[0].hwnd;
    }

    std::cout << "Window not found. Attempting to start application: " << app_path << std::endl;

    // 将 std::string 转换为宽字符 std::wstring
    // 首先获取所需的缓冲区大小
    int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, app_path.c_str(), -1, NULL, 0);
    if (wideCharLength == 0) {
        std::cerr << "Error converting path to wide string." << std::endl;
        return nullptr; // Return null handle on error
    }

    // 创建宽字符缓冲区并转换
    std::vector<wchar_t> widePath(wideCharLength);
    MultiByteToWideChar(CP_UTF8, 0, app_path.c_str(), -1, widePath.data(), wideCharLength);

    // 使用 ShellExecuteW 启动程序
    HINSTANCE result = ShellExecuteW(NULL, L"open", widePath.data(), NULL, NULL, SW_SHOWNORMAL);

    // 检查 ShellExecute 是否成功启动了程序
    // 成功时，返回值通常是一个大于 32 的句柄
    if ((uintptr_t)result <= 32) {
        std::cerr << "Failed to start application. Error code: " << (uintptr_t)result << std::endl;
        return nullptr; // Return null handle on failure
    }

    std::cout << "Application started successfully." << std::endl;

    // 启动后等待一小段时间，让程序有机会创建窗口
    SleepMs(2000); // Wait 2 seconds

    // 再次尝试查找窗口
    auto newWindows = FindWindows(title, class_name);
    if (!newWindows.empty()) {
        std::cout << "New window found after starting app: " << newWindows[0].title << std::endl;
        return newWindows[0].hwnd;
    }
    else {
        std::cout << "Could not find the window after starting the application." << std::endl;
        return nullptr; // Return null handle if still not found
    }
}


// 菜单相关实现 (与之前相同)
bool Win32Util::ClickMenuItem(HWND hwnd, const std::string& menu_text) {
    HMENU hmenu = GetMenu(hwnd);
    if (!hmenu) {
        std::cerr << "No main menu found for window." << std::endl;
        return false;
    }

    int count = GetMenuItemCount(hmenu);
    for (int i = 0; i < count; ++i) {
        char buffer[256];
        MENUITEMINFOA mii;
        ZeroMemory(&mii, sizeof(MENUITEMINFOA));
        mii.cbSize = sizeof(MENUITEMINFOA);
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.dwTypeData = buffer;
        mii.cch = sizeof(buffer) - 1;

        if (GetMenuItemInfoA(hmenu, i, TRUE, &mii)) {
            if (mii.fType == MFT_STRING && std::string(buffer) == menu_text) {
                PostMessageA(hwnd, WM_COMMAND, mii.wID, 0);
                return true;
            }
        }
    }
    return false;
}

bool Win32Util::ClickMenuByIndex(HWND hwnd, int item_index) {
    HMENU hmenu = GetMenu(hwnd);
    if (!hmenu) {
        std::cerr << "No main menu found for window." << std::endl;
        return false;
    }

    int count = GetMenuItemCount(hmenu);
    if (item_index >= 0 && item_index < count) {
        MENUITEMINFOA mii;
        ZeroMemory(&mii, sizeof(MENUITEMINFOA));
        mii.cbSize = sizeof(MENUITEMINFOA);
        mii.fMask = MIIM_ID;

        if (GetMenuItemInfoA(hmenu, item_index, TRUE, &mii)) {
            PostMessageA(hwnd, WM_COMMAND, mii.wID, 0);
            return true;
        }
    }
    return false;
}

// 输入模拟实现 (与之前相同)
void Win32Util::MouseClick(int x, int y) const {
    SetCursorPos(x, y);
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void Win32Util::MouseRightClick(int x, int y) const {
    SetCursorPos(x, y);
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP;
    SendInput(1, &input, sizeof(INPUT));
}

void Win32Util::SendKey(char key) const {
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VkKeyScanA(key);
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(INPUT));

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void Win32Util::SendText(const std::string& text) const {
    for (char c : text) {
        SendKey(c);
    }
}

// --- 截图实现 (使用 GDI+) ---
bool Win32Util::CaptureScreenToBMPFile(const std::string& filename) const {
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldbmp = SelectObject(hMemDC, hBitmap);

    BitBlt(hMemDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);

    bool success = SaveHBITMAPToFile(hBitmap, filename);

    SelectObject(hMemDC, oldbmp); // Restore original bitmap
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);

    return success;
}

bool Win32Util::CaptureWindowToBMPFile(HWND hwnd, const std::string& filename) const {
    RECT rect;
    if (!GetWindowRect(hwnd, rect)) {
        return false;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    return CaptureRegionToBMPFile(rect.left, rect.top, width, height, filename);
}

bool Win32Util::CaptureRegionToBMPFile(int x, int y, int width, int height, const std::string& filename) const {
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldbmp = SelectObject(hMemDC, hBitmap);

    BitBlt(hMemDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY);

    bool success = SaveHBITMAPToFile(hBitmap, filename);

    SelectObject(hMemDC, oldbmp);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);

    return success;
}

// 获取原始像素数据 (BGR 24bpp)
std::vector<BYTE> Win32Util::CaptureScreenRaw(int& width, int& height) const {
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldbmp = SelectObject(hMemDC, hBitmap);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24; // BGR
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<BYTE> pixels(width * height * 3); // 3 bytes per pixel (BGR)

    GetDIBits(hMemDC, hBitmap, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

    SelectObject(hMemDC, oldbmp);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);

    return pixels;
}

std::vector<BYTE> Win32Util::CaptureWindowRaw(HWND hwnd, int& width, int& height) const {
    RECT rect;
    if (!GetWindowRect(hwnd, rect)) {
        width = 0; height = 0;
        return {};
    }
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    return CaptureRegionRaw(rect.left, rect.top, width, height, width, height);
}

std::vector<BYTE> Win32Util::CaptureRegionRaw(int x, int y, int width, int height, int& width_out, int& height_out) const {
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldbmp = SelectObject(hMemDC, hBitmap);

    BitBlt(hMemDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24; // BGR
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<BYTE> pixels(width * height * 3); // 3 bytes per pixel (BGR)

    GetDIBits(hMemDC, hBitmap, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

    SelectObject(hMemDC, oldbmp);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);

    width_out = width;
    height_out = height;
    return pixels;
}

// 保存 HBITMAP 到 BMP 文件
bool Win32Util::SaveHBITMAPToFile(HBITMAP hBitmap, const std::string& filename) const {
    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = bmp.bmBitsPixel;
    bi.biCompression = BI_RGB;

    DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
    HANDLE hDib = GlobalAlloc(GHND, dwBmpSize);
    char* lpBuf = (char*)GlobalLock(hDib);

    HDC hdc = GetDC(NULL);
    GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, lpBuf, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    BITMAPFILEHEADER bmfHdr = { 0 };
    bmfHdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHdr.bfSize = bmfHdr.bfOffBits + dwBmpSize;
    bmfHdr.bfType = 0x4D42; // BM

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        GlobalUnlock(hDib);
        GlobalFree(hDib);
        return false;
    }

    file.write(reinterpret_cast<const char*>(&bmfHdr), sizeof(BITMAPFILEHEADER));
    file.write(reinterpret_cast<const char*>(&bi), sizeof(BITMAPINFOHEADER));
    file.write(lpBuf, dwBmpSize);

    file.close();
    GlobalUnlock(hDib);
    GlobalFree(hDib);

    return true;
}

// --- 简单图像比较 (非模板匹配) ---
bool Win32Util::ComparePixelAt(HWND hwnd, int x, int y, COLORREF target_color, int tolerance) const {
    HDC hdc = GetDC(hwnd);
    if (!hdc) return false;

    COLORREF actual_color = GetPixel(hdc, x, y);
    ReleaseDC(hwnd, hdc);

    // 简单的颜色分量比较
    int r_diff = abs(GetRValue(actual_color) - GetRValue(target_color));
    int g_diff = abs(GetGValue(actual_color) - GetGValue(target_color));
    int b_diff = abs(GetBValue(actual_color) - GetBValue(target_color));

    return (r_diff <= tolerance && g_diff <= tolerance && b_diff <= tolerance);
}

// 辅助函数实现 (与之前相同)
void Win32Util::SleepMs(int milliseconds) const {
    ::Sleep(milliseconds);
}