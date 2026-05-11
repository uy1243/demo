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
#include <io.h>
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

HWND Win32Util::FindWindowOrStartApp(const std::string& title, const std::string& class_name, const std::string& app_path) {
    // 1. 先尝试查找窗口
    auto windows = FindWindows(title, class_name);
    if (!windows.empty()) {
        std::cout << "Found existing window: " << windows[0].title << std::endl;
        return windows[0].hwnd;
    }

    std::cout << "Window not found. Attempting to start application: " << app_path << std::endl;

    // 2. 【新增】检查文件是否真的存在
    // _access 返回 0 表示文件存在
    //if (_access(app_path.c_str(), 0) != 0) {
    //    std::cerr << "Error: File does not exist at path: " << app_path << std::endl;
    //    // 如果路径看起来是绝对的但文件不存在，尝试给出建议
    //    if (app_path.find("notepad") != std::string::npos) {
    //        std::cerr << "Hint: Try using the full path, e.g., C:\\Windows\\System32\\notepad.exe" << std::endl;
    //    }
    //    return nullptr;
    //}

    // 3. 转换字符串
    int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, app_path.c_str(), -1, NULL, 0);
    if (wideCharLength == 0) {
        std::cerr << "Error converting path to wide string." << std::endl;
        return nullptr;
    }

    std::vector<wchar_t> widePath(wideCharLength);
    MultiByteToWideChar(CP_UTF8, 0, app_path.c_str(), -1, widePath.data(), wideCharLength);

    // 4. 启动程序
    // 使用 widePath.data() 作为 lpFile
    HINSTANCE result = ShellExecuteW(NULL, L"open", widePath.data(), NULL, NULL, SW_SHOWNORMAL);

    // 5. 检查返回值
    if ((uintptr_t)result <= 32) {
        std::cerr << "Failed to start application. Error code: " << (uintptr_t)result << std::endl;

        // 常见错误码解释
        switch ((uintptr_t)result) {
        case 2: std::cerr << "Detail: The system cannot find the file specified (ERROR_FILE_NOT_FOUND)." << std::endl; break;
        case 3: std::cerr << "Detail: The system cannot find the path specified (ERROR_PATH_NOT_FOUND)." << std::endl; break;
        case 5: std::cerr << "Detail: Access denied (ERROR_ACCESS_DENIED)." << std::endl; break;
        case 11: std::cerr << "Detail: Attempted to load a bad .exe file (ERROR_BAD_FORMAT)." << std::endl; break;
        }
        return nullptr;
    }

    std::cout << "Application started successfully." << std::endl;

    // 6. 等待并再次查找
    SleepMs(2000);
    auto newWindows = FindWindows(title, class_name);
    if (!newWindows.empty()) {
        std::cout << "New window found: " << newWindows[0].title << std::endl;
        return newWindows[0].hwnd;
    }

    return nullptr;
}


// 菜单相关实现 (修改版)
bool Win32Util::ClickMenuItem(HWND hwnd, const std::string& menu_path) {
    HMENU hMainmenu = GetMenu(hwnd);
    if (!hMainmenu) {
        std::cerr << "No main menu found for window." << std::endl;
        return false;
    }

    // 使用 "->" 分割菜单路径
    std::vector<std::string> parts;
    std::string current_part;
    for (char c : menu_path) {
        if (c == '-' && !current_part.empty() && current_part.back() == '>') {
            // 处理 "->" 分隔符
            if (parts.empty()) {
                parts.push_back(current_part.substr(0, current_part.length() - 1)); // 移除最后的 '>'
            }
            else {
                // 这种情况理论上不应该出现，但为了健壮性
                current_part += c;
            }
        }
        else if (c == '>' && !current_part.empty() && current_part.back() == '-') {
            // 这种情况理论上不应该出现，但为了健壮性
            current_part += c;
        }
        else {
            if (c == '>' && current_part.length() > 0 && current_part.back() == '-') {
                // 正确的 "->" 分隔符
                parts.push_back(current_part.substr(0, current_part.length() - 1)); // 移除 "->"
                current_part.clear(); // 开始下一个部分
            }
            else {
                current_part += c;
            }
        }
    }
    if (!current_part.empty()) {
        parts.push_back(current_part);
    }

    if (parts.empty()) {
        std::cerr << "Invalid menu path format." << std::endl;
        return false;
    }

    HMENU current_menu = hMainmenu;
    UINT final_command_id = 0;
    bool found = true;

    for (size_t i = 0; i < parts.size(); ++i) {
        const std::string& target_text = parts[i];
        int count = GetMenuItemCount(current_menu);
        bool item_found = false;

        for (int j = 0; j < count; ++j) {
            char buffer[256];
            MENUITEMINFOA mii;
            ZeroMemory(&mii, sizeof(MENUITEMINFOA));
            mii.cbSize = sizeof(MENUITEMINFOA);
            mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU; // 请求类型、ID和子菜单句柄
            mii.dwTypeData = buffer;
            mii.cch = sizeof(buffer) - 1;

            if (GetMenuItemInfoA(current_menu, j, TRUE, &mii)) { // TRUE 表示按位置索引
                if (mii.fType == MFT_STRING && std::string(buffer) == target_text) {
                    if (i == parts.size() - 1) { // 如果是最后一个部分
                        if (mii.hSubMenu) {
                            // 如果最后一个部分是子菜单而不是命令，这可能是错误的
                            std::cerr << "Final menu item '" << target_text << "' is a submenu, not a command." << std::endl;
                            return false;
                        }
                        final_command_id = mii.wID;
                        item_found = true;
                        break;
                    }
                    else { // 如果不是最后一个部分
                        if (!mii.hSubMenu) {
                            // 如果中间部分没有子菜单，说明路径错误
                            std::cerr << "Menu item '" << target_text << "' does not have a submenu." << std::endl;
                            found = false;
                            break;
                        }
                        current_menu = mii.hSubMenu; // 进入下一级菜单
                        item_found = true;
                        break;
                    }
                }
            }
        }

        if (!item_found || !found) {
            found = false;
            break;
        }
    }

    if (found && final_command_id != 0) {
        PostMessageA(hwnd, WM_COMMAND, final_command_id, 0);
        return true;
    }
    else {
        std::cerr << "Menu path '" << menu_path << "' not found." << std::endl;
        return false;
    }
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

/**
 * @brief 查找 TXPButton
 */
HWND Win32Util::FindTXPButton(HWND parent_hwnd, const std::string& caption) const {
    if (!parent_hwnd) {
        // 如果父句柄无效，尝试在整个桌面查找（通常不推荐，除非知道它在顶层）
        parent_hwnd = HWND_MESSAGE; // 或者 NULL
    }

    // 将 std::string 转为 LPCSTR
    LPCSTR class_name = "TXPButton";
    LPCSTR window_title = caption.empty() ? NULL : caption.c_str();

    // 查找第一个匹配的子窗口
    HWND hwnd = FindWindowExA(parent_hwnd, NULL, class_name, window_title);

    if (!hwnd) {
        std::cerr << "[Win32Util] TXPButton not found: " << (caption.empty() ? "(No Caption)" : caption) << std::endl;
    }
    return hwnd;
}

/**
 * @brief 点击 TXPButton
 * 注意：自定义控件通常不响应标准的 BM_CLICK，需要模拟鼠标消息
 */
bool Win32Util::ClickTXPButton(HWND button_hwnd) const {
    if (!IsWindow(button_hwnd)) {
        std::cerr << "[Win32Util] Invalid button handle." << std::endl;
        return false;
    }

    // 1. 尝试获取焦点 (很多自定义控件必须有焦点才响应)
    SetFocus(button_hwnd);

    // 发送 WM_SETFOCUS 消息强制激活
    SendMessage(button_hwnd, WM_SETFOCUS, 0, 0);

    Sleep(100); // 稍微等待UI线程处理焦点

    // 2. 模拟鼠标点击 (最通用的方法)
    // 构造 LPARAM (x, y 坐标)，这里设为 (1, 1) 表示在控件客户区内点击
    LPARAM lParam = MAKELPARAM(1, 1);

    // 发送按下消息
    SendMessage(button_hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);

    // 发送抬起消息
    SendMessage(button_hwnd, WM_LBUTTONUP, 0, lParam);

    // 3. 额外尝试：发送 WM_COMMAND 给父窗口
    // 有些控件点击后只是通知父窗口，我们需要手动触发这个通知
    HWND parent = GetParent(button_hwnd);
    if (parent) {
        // 获取控件 ID
        UINT_PTR id = GetWindowLongPtr(button_hwnd, GWLP_ID);
        // 发送 BN_CLICKED 通知码
        SendMessage(parent, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)button_hwnd);
    }

    return true;
}

/**
 * @brief 查找并点击
 */
bool Win32Util::FindAndClickTXPButton(HWND parent_hwnd, const std::string& caption) const {
    HWND btn = FindTXPButton(parent_hwnd, caption);
    if (btn) {
        return ClickTXPButton(btn);
    }
    return false;
}

bool Win32Util::ClickAtCoordinate(HWND hwnd, int x, int y) const {
    if (!IsWindow(hwnd)) return false;

    // 1. 获取窗口在屏幕上的绝对位置
    RECT rect;
    if (!GetWindowRect(hwnd, rect))
    {
		std::cout << " Failed to get window rect." << std::endl;
        return false;
    }

    // 2. 计算屏幕绝对坐标 (相对坐标 + 窗口左上角坐标)
    int screen_x = rect.left + x;
    int screen_y = rect.top + y;

    // 3. 备份当前鼠标位置 (可选，为了用户体验)
    POINT old_pos;
    GetCursorPos(&old_pos);

    // 4. 移动鼠标到目标位置
    SetCursorPos(screen_x, screen_y);

    // 5. 稍微等待鼠标移动到位
    Sleep(50);

    // 6. 模拟鼠标左键按下和抬起 (物理点击)
    // 注意：这里使用 mouse_event，虽然在新版Windows中推荐 SendInput，但 mouse_event 兼容性极好
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    Sleep(50); // 模拟按住一小会儿
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

    // 7. 恢复鼠标位置 (可选)
    // SetCursorPos(old_pos.x, old_pos.y);

    return true;
}

void Win32Util::EnumerateChildControls(HWND parent_hwnd, const std::string& class_name) const {
    HWND current = NULL;
    RECT parent_rect;
    GetWindowRect(parent_hwnd, parent_rect); // 获取父窗口绝对位置用于计算相对坐标

    std::cout << "--- 枚举控件: " << class_name << " ---" << std::endl;
    int index = 0;

    while ((current = FindWindowExA(parent_hwnd, current, class_name.c_str(), NULL)) != NULL) {
        RECT rect;
        if (GetWindowRect(current, rect)) {
            // 计算相对坐标
            int relative_x = rect.left - parent_rect.left;
            int relative_y = rect.top - parent_rect.top;
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            char title[256];
            GetWindowTextA(current, title, sizeof(title));

            std::cout << "[" << index << "] 标题=\"" << title
                << "\", 相对坐标=(" << relative_x << ", " << relative_y << ")"
                << ", 大小=" << width << "x" << height << std::endl;
            index++;
        }
    }
    std::cout << "--- 枚举结束 ---" << std::endl;
}