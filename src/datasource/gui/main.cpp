#include <iostream>
#include <windows.h>
#include <io.h> 
#include <fcntl.h> 
#include <cwchar> 
#include <clocale> 
#include <string> // For std::wstring
#include <fstream>
#include <gdiplus.h>
#include <windows.h>
#include <gdiplus.h>
#include <winuser.h>
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <gdiplus.h>
// --- 添加以下头文件 ---
#include <objidl.h>  // 包含 CLSID 定义
// -------------------

#include <winuser.h>
#include <iostream>
#include <vector>
#include <string>
#include <locale>

using namespace Gdiplus; // 推荐使用命名空间，避免写 Gdiplus::Graphics 等
#include <locale>
#pragma comment(lib, "Gdiplus.lib")

// GDI+ 用于保存高质量图片
ULONG_PTR gdiplusToken = 0;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j; // 返回找到的索引，通常用不到
        }
    }

    free(pImageCodecInfo);
    return -1; // 没找到
}
// ------------------------------------------


void InitializeGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void ShutdownGDIPlus() {
    Gdiplus::GdiplusShutdown(gdiplusToken);
}

// 使用 PrintWindow 和 GDI+ 保存窗口截图 (支持多种格式备选)
bool CaptureAndSaveWindowWithGDIPlus(HWND hwnd, const wchar_t* baseFilename) {
    if (!IsWindow(hwnd)) {
        std::wcout << L"窗口句柄无效。" << std::endl;
        return false;
    }

    // 获取窗口尺寸（包含边框和标题栏）
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;

    if (width <= 0 || height <= 0) {
        std::wcout << L"窗口尺寸无效。" << std::endl;
        return false;
    }

    // 创建内存 DC 和位图
    HDC hdcScreen = GetDC(hwnd); // 获取窗口DC
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdcScreen, width, height);
    if (!hbmMem || !hdcMem) {
        std::wcout << L"创建内存DC或位图失败。" << std::endl;
        if (hdcMem) DeleteDC(hdcMem);
        if (hbmMem) DeleteObject(hbmMem);
        ReleaseDC(hwnd, hdcScreen);
        return false;
    }

    HGDIOBJ oldBitmap = SelectObject(hdcMem, hbmMem);

    // 使用 PrintWindow 捕获窗口内容
    BOOL bRet = PrintWindow(hwnd, hdcMem, PW_RENDERFULLCONTENT);
    if (!bRet) {
        std::wcout << L"PrintWindow 失败。错误码: " << GetLastError() << std::endl;
        SelectObject(hdcMem, oldBitmap);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcScreen);
        return false;
    }

    // 将 HBITMAP 转换为 GDI+ Bitmap
    Gdiplus::Bitmap bitmap(hbmMem, NULL);

    // 尝试保存格式列表
    const wchar_t* formats[] = { L"image/png", L"image/jpeg", L"image/bmp" };
    const wchar_t* extensions[] = { L".png", L".jpg", L".bmp" };

    bool savedSuccessfully = false;
    for (int i = 0; i < 3; ++i) {
        CLSID encoderClsid;
        if (GetEncoderClsid(formats[i], &encoderClsid) == -1) {
            std::wcout << L"未找到 " << formats[i] << L" 编码器，跳过。" << std::endl;
            continue; // 尝试下一个格式
        }

        // 构造带扩展名的文件名
        std::wstring fullFilename = baseFilename;
        fullFilename += extensions[i];

        Status saveStatus = bitmap.Save(fullFilename.c_str(), &encoderClsid, NULL);
        if (saveStatus == Ok) {
            std::wcout << L"GDI+ 图片保存成功: " << fullFilename << std::endl;
            savedSuccessfully = true;
            break; // 成功就退出循环
        }
        else {
            std::wcout << L"尝试保存为 " << formats[i] << L" 失败，状态: " << saveStatus << std::endl;
        }
    }

    if (!savedSuccessfully) {
        std::wcout << L"❌ 所有可用格式均保存失败！" << std::endl;
    }

    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcScreen);
    return savedSuccessfully; // 返回是否成功保存过至少一种格式
}

// 修正 GetMenuItemScreenRect 函数签名，需要窗口句柄
bool GetMenuItemScreenRect(HWND hwnd, HMENU hMenu, int itemIndex, LPRECT lpRect) {
    if (!hwnd || !hMenu || itemIndex < 0) return false;
    // 使用 TrackPopupMenuEx 创建一个临时的、可见的菜单来获取位置，但这很复杂
    // 更简单的方法是使用 GetMenuItemRect，但它需要窗口句柄
    // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmenuitemrect
    // BOOL GetMenuItemRect(
    //   HWND    hWnd,  // 这里需要顶级窗口句柄
    //   HMENU   hMenu, // 子菜单句柄
    //   UINT    uItem, // 子菜单项索引
    //   LPRECT  lprc   // 输出矩形
    // );
    if (!GetMenuItemRect(hwnd, hMenu, itemIndex, lpRect)) {
         // 如果直接获取失败，可能是菜单未完全渲染或不是标准菜单
         // 这种情况下很难精确获取单个项的位置
         // GetLastError() 可以提供更多线索
         return false;
    }
    // GetMenuItemRect 返回的是屏幕坐标，直接可用
    return true;
}

// 保存窗口区域函数保持不变
bool SaveWindowRegionToBmp(HWND hwnd, RECT rect, const wchar_t* filename) {
    HDC hScreenDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, rect.right - rect.left, rect.bottom - rect.top);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);

    BitBlt(hMemDC, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
           hScreenDC, rect.left, rect.top, SRCCOPY);

    BITMAPFILEHEADER bfh = {0};
    BITMAPINFOHEADER bih = {0};
    bfh.bfType = 0x4D42; // 'BM'
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = rect.right - rect.left;
    bih.biHeight = -(rect.bottom - rect.top); // negative for top-down DIB
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DeleteObject(hBitmap);
        DeleteDC(hMemDC);
        ReleaseDC(NULL, hScreenDC);
        return false;
    }

    DWORD written;
    WriteFile(hFile, &bfh, sizeof(bfh), &written, NULL);
    WriteFile(hFile, &bih, sizeof(bih), &written, NULL);
    WriteFile(hFile, hBitmap, (rect.right - rect.left) * (rect.bottom - rect.top) * 4, &written, NULL);

    CloseHandle(hFile);
    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hScreenDC);
    return true;
}

std::string OCRRecongize(const char* imgPath)
{
    // tesseract 路径 + 命令行：识别图片输出到txt
    std::string cmd =
        R"("C:\Program Files\Tesseract-OCR\tesseract.exe" )"
        + std::string(imgPath) + " ocr_out -l chi_sim+eng";

    system(cmd.c_str());

    // 读取输出的 ocr_out.txt
    std::ifstream fin("ocr_out.txt");
    std::string res, line;
    while (getline(fin, line)) res += line + "\n";
    return res;
}

// 定义回调函数，用于枚举窗口
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    wchar_t title[256] = { 0 };
    int len = GetWindowTextW(hwnd, title, 256);
    if (len > 0) {
        wprintf(L"句柄: 0x%p | 标题: %ls\n", hwnd, title);
        fflush(stdout);
    }
    return TRUE;
}

// 递归函数，用于打印子菜单项
void PrintSubMenu(HMENU hSubMenu, int level = 1) {
    if (!hSubMenu) return;

    int subItemCount = GetMenuItemCount(hSubMenu);
    for (int j = 0; j < subItemCount; ++j) {
        wchar_t subMenuText[256] = { 0 };
        // 尝试获取子菜单项文本
        int length = GetMenuStringW(hSubMenu, j, subMenuText, 256, MF_BYPOSITION);

        if (length > 0) {
            // 打印带缩进的子菜单项
            for (int k = 0; k < level; ++k) wprintf(L"  ");
            wprintf(L"-> 子菜单项 [%d]: %ls\n", j, subMenuText);
            fflush(stdout);
        }
        else {
            // 尝试获取子菜单的句柄 (如果有下一级菜单)
            HMENU subSubHandle = GetSubMenu(hSubMenu, j);
            if (subSubHandle) {
                for (int k = 0; k < level; ++k) wprintf(L"  ");
                wprintf(L"-> 子菜单项 [%d]: (进入下级菜单)\n", j);
                PrintSubMenu(subSubHandle, level + 1); // 递归打印下一级
            }
            else {
                for (int k = 0; k < level; ++k) wprintf(L"  ");
                std::wcout << L"-> 子菜单项 [" << j << L"]: (非字符串项或获取失败)" << std::endl;
            }
        }
    }
}


int main0() {
    setlocale(LC_ALL, "");

    std::wcout << L"=== 开始枚举所有顶层窗口 ===" << std::endl;
    EnumWindows(EnumWindowsProc, 0);
    std::wcout << L"\n=== 枚举结束 ===" << std::endl;

    // --- 修改：使用完整标题查找窗口 ---
    std::wcout << L"\n--- 正在查找 'Vector CANdb++ Editor' 窗口 (精确匹配) ---" << std::endl;
    const wchar_t* fullWindowTitle = L"Vector CANdb++ Editor - C:\\Users\\Administrator\\Desktop\\ADASDBC_receive.dbc - [Overall View]";
    HWND vectorHwnd = FindWindowW(NULL, fullWindowTitle);

    if (vectorHwnd == NULL) {
        // 如果精确匹配失败，尝试模糊匹配
        std::wcout << L"精确匹配失败，正在尝试模糊匹配..." << std::endl;

        // Lambda 捕获外部变量 vectorHwnd 来存储找到的句柄
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            wchar_t title[256] = { 0 };
            int len = GetWindowTextW(hwnd, title, 256);
            if (len > 0 && wcsstr(title, L"Vector CANdb++ Editor") != nullptr) {
                std::wcout << L"模糊匹配找到: 句柄 0x" << std::hex << hwnd << L" | 标题: " << title << std::endl;
                // 通过指针修改外部变量
                *reinterpret_cast<HWND*>(lParam) = hwnd;
                return FALSE; // 找到后停止枚举
            }
            return TRUE;
            }, reinterpret_cast<LPARAM>(&vectorHwnd));
    }

    if (vectorHwnd != NULL) {
        std::wcout << L"\n--- 找到窗口! 句柄: 0x" << std::hex << vectorHwnd << L" ---" << std::endl;

        // 获取窗口类名
        wchar_t className[256];
        GetClassNameW(vectorHwnd, className, 256);
        std::wcout << L"窗口类名: " << className << std::endl;

        // 获取菜单句柄
        HMENU hMenu = GetMenu(vectorHwnd);
        if (hMenu != NULL) {
            std::wcout << L"\n--- 获取到主菜单句柄 ---" << std::endl;

            int menuItemCount = GetMenuItemCount(hMenu);
            std::wcout << L"顶级菜单项总数: " << menuItemCount << std::endl;

            // 遍历并打印顶级菜单项及其子菜单
            for (int i = 0; i < menuItemCount; ++i) {
                wchar_t menuText[256] = { 0 };
                int length = GetMenuStringW(hMenu, i, menuText, 256, MF_BYPOSITION);

                if (length > 0) {
                    wprintf(L"\n顶级菜单项 [%d]: %ls\n", i, menuText);

                    // 尝试获取该顶级菜单项的子菜单句柄
                    HMENU subMenuHandle = GetSubMenu(hMenu, i);
                    if (subMenuHandle) {
                        std::wcout << L"  (发现子菜单，开始打印子项)" << std::endl;
                        PrintSubMenu(subMenuHandle, 1); // 从第1层开始打印
                    }
                    else {
                        std::wcout << L"  (无子菜单)" << std::endl;
                    }

                }
                else {
                    // 可能是分隔符
                    std::wcout << L"顶级菜单项 [" << i << L"]: (非字符串项或获取失败)" << std::endl;
                }
            }
        }
        else {
            std::wcout << L"窗口没有菜单或获取菜单失败。错误码: " << GetLastError() << std::endl;
        }

    }
    else {
        std::wcout << L"\n--- 未能找到 'Vector CANdb++ Editor' 窗口 ---" << std::endl;
    }

    std::wcout << L"\n--- 程序结束 ---" << std::endl;
    system("pause");
    return 0;
}

#include <iostream>
#include <windows.h>
#include <io.h> 
#include <fcntl.h> 
#include <cwchar> 
#include <clocale> 
#include <string>
#include <vector>

struct MenuItemInfo {
    std::vector<std::wstring> path; // e.g., {"File", "Create Database..."}
    UINT commandId;                 // e.g., 0x8CA0
};

void CollectMenuItems(HMENU hMenu, const std::vector<std::wstring>& parentPath, std::vector<MenuItemInfo>& allItems) {
    if (!hMenu) return;

    int itemCount = GetMenuItemCount(hMenu);
    for (int i = 0; i < itemCount; ++i) {
        wchar_t menuText[256] = { 0 };
        int length = GetMenuStringW(hMenu, i, menuText, 256, MF_BYPOSITION);

        if (length > 0) {
            std::vector<std::wstring> currentPath = parentPath;
            currentPath.push_back(std::wstring(menuText));

            UINT itemId = GetMenuItemID(hMenu, i);
            if (itemId != -1) {
                MenuItemInfo item;
                item.path = currentPath;
                item.commandId = itemId;
                allItems.push_back(item);
            }
            else {
                HMENU subMenuHandle = GetSubMenu(hMenu, i);
                if (subMenuHandle) {
                    CollectMenuItems(subMenuHandle, currentPath, allItems);
                }
            }
        }
        else {
            HMENU subMenuHandle = GetSubMenu(hMenu, i);
            if (subMenuHandle) {
                CollectMenuItems(subMenuHandle, parentPath, allItems);
            }
        }
    }
}

int main1() {
    setlocale(LC_ALL, "");
    OCRRecongize("C:\\Users\\Administrator\\Pictures\\1.png");
    std::wcout << L"--- 正在查找 'Vector CANdb++ Editor' 窗口 ---" << std::endl;
    HWND vectorHwnd = NULL;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        wchar_t title[256] = { 0 };
        int len = GetWindowTextW(hwnd, title, 256);
        if (len > 0 && wcsstr(title, L"Vector CANdb++ Editor") != nullptr) {
            std::wcout << L"模糊匹配找到: 句柄 0x" << std::hex << hwnd << L" | 标题: " << title << std::endl;
            *reinterpret_cast<HWND*>(lParam) = hwnd;
            return FALSE;
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&vectorHwnd));

    if (vectorHwnd != NULL) {
        std::wcout << L"\n--- 找到窗口! 句柄: 0x" << std::hex << vectorHwnd << L" ---" << std::endl;

        HMENU hMenu = GetMenu(vectorHwnd);
        if (hMenu != NULL) {
            std::wcout << L"\n--- 开始收集菜单项信息 ---" << std::endl;

            std::vector<std::wstring> initialParentPath;
            std::vector<MenuItemInfo> allMenuItems;

            CollectMenuItems(hMenu, initialParentPath, allMenuItems);

            std::wcout << L"共收集到 " << allMenuItems.size() << L" 个可点击的菜单项。" << std::endl;

            std::wstring targetTopLevelName = L"&File"; // 修正：匹配实际输出的 "&File"
            std::wstring targetSubPrefix = L"Create Data";
            UINT targetCommandId = 0;
            std::wstring matchedSubName;
            int topLevelIndex = -1; // 记录顶级菜单项的索引
            int subItemIndex = -1;  // 记录子菜单项的索引

            // --- 第一步：查找目标菜单项，并记录其路径索引 ---
            for (size_t itemIdx = 0; itemIdx < allMenuItems.size(); ++itemIdx) {
                const auto& item = allMenuItems[itemIdx];
                if (item.path.size() != 2) continue;
                if (item.path[0] != targetTopLevelName) continue;

                if (item.path[1].length() >= targetSubPrefix.length() &&
                    item.path[1].compare(0, targetSubPrefix.length(), targetSubPrefix) == 0) {

                    targetCommandId = item.commandId;
                    matchedSubName = item.path[1];

                    // 重新计算索引（因为路径是从收集过程推断的，我们需要实际的索引）
                    // 遍历主菜单找到 "&File" 的索引
                    int count = GetMenuItemCount(hMenu);
                    for (int i = 0; i < count; ++i) {
                        wchar_t tempText[256] = { 0 };
                        if (GetMenuStringW(hMenu, i, tempText, 256, MF_BYPOSITION) > 0) {
                            if (std::wstring(tempText) == targetTopLevelName) {
                                topLevelIndex = i;
                                break;
                            }
                        }
                    }

                    if (topLevelIndex != -1) {
                        HMENU subMenu = GetSubMenu(hMenu, topLevelIndex);
                        if (subMenu) {
                            // 遍历子菜单找到匹配项的索引
                            int subCount = GetMenuItemCount(subMenu);
                            for (int j = 0; j < subCount; ++j) {
                                wchar_t tempSubText[256] = { 0 };
                                if (GetMenuStringW(subMenu, j, tempSubText, 256, MF_BYPOSITION) > 0) {
                                    if (std::wstring(tempSubText) == matchedSubName) {
                                        subItemIndex = j;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    std::wcout << L"找到目标菜单项 '" << targetTopLevelName << L"' -> '" << matchedSubName << L"'!" << std::endl;
                    std::wcout << L"命令ID: 0x" << std::hex << targetCommandId << std::endl;
                    std::wcout << L"顶级菜单索引: " << topLevelIndex << L", 子菜单索引: " << subItemIndex << std::endl;
                    break;
                }
            }

            if (targetCommandId != 0 && topLevelIndex != -1 && subItemIndex != -1) {

                // --- 第二步：触发菜单展开 ---
                wchar_t topLevelText[256] = { 0 };
                GetMenuStringW(hMenu, topLevelIndex, topLevelText, 256, MF_BYPOSITION);
                wchar_t mnemonicChar = 0;
                for (wchar_t* p = topLevelText; *p; ++p) {
                    if (*p == L'&' && *(p + 1)) {
                        mnemonicChar = *(p + 1);
                        break;
                    }
                }

                if (mnemonicChar) {
                    std::wcout << L"模拟按下 Alt+" << mnemonicChar << L" 以展开菜单..." << std::endl;
                    PostMessageW(vectorHwnd, WM_SYSCOMMAND, SC_KEYMENU, (LPARAM)mnemonicChar);
                    Sleep(1000); // 等待菜单展开
                }
                else {
                    std::wcout << L"警告: 无法找到顶级菜单的助记符。" << std::endl;
                }

                // --- 第三步：由于无法精确获取子菜单项区域，改为截取整个窗口 ---
                std::wcout << L"⚠️  无法精确获取菜单项区域，正在截取整个窗口..." << std::endl;

                // 获取窗口客户区大小 (不含边框和标题栏)
                RECT clientRect;
                GetClientRect(vectorHwnd, &clientRect);

                // 将客户区坐标转换为屏幕坐标
                POINT topLeft = { clientRect.left, clientRect.top };
                ClientToScreen(vectorHwnd, &topLeft);

                RECT windowRect = { topLeft.x, topLeft.y, topLeft.x + (clientRect.right - clientRect.left), topLeft.y + (clientRect.bottom - clientRect.top) };

                std::wstring bmpPath = L"window_capture_with_menu.bmp";
                if (SaveWindowRegionToBmp(NULL, windowRect, bmpPath.c_str())) {
                    std::wcout << L"✅ 窗口截图（包含展开的菜单）已保存至: " << bmpPath << std::endl;
                }
                else {
                    std::wcout << L"❌ 窗口截图保存失败！" << std::endl;
                }

                // --- 第四步：发送命令点击 ---
                std::wcout << L"\n--- 尝试发送 WM_COMMAND 消息以点击菜单项 ---" << std::endl;
                WPARAM wParam = MAKEWPARAM(targetCommandId, 0);
                LPARAM lParam = 0;
                PostMessageW(vectorHwnd, WM_COMMAND, wParam, lParam);

                std::wcout << L"WM_COMMAND 消息已发送，可能已触发 '" << matchedSubName << L"'。" << std::endl;

            }
            else {
                std::wcout << L"\n--- 未找到匹配的菜单项或无法确定其索引 'File' -> 以 '" << targetSubPrefix << L"' 开头 ---" << std::endl;
                std::wcout << L"以下是 'File' 菜单下的所有可点击子项:" << std::endl;
                for (const auto& item : allMenuItems) {
                    if (item.path.size() == 2 && item.path[0] == L"&File") { // 修正：匹配 "&File"
                        wprintf(L"  '%ls' (ID: 0x%X)\n", item.path[1].c_str(), item.commandId);
                    }
                }
            }

        }
        else {
            std::wcout << L"窗口没有菜单或获取菜单失败。错误码: " << GetLastError() << std::endl;
        }

    }
    else {
        std::wcout << L"\n--- 未能找到 'Vector CANdb++ Editor' 窗口 ---" << std::endl;
    }

    std::wcout << L"\n--- 程序结束 ---" << std::endl;
    system("pause");
    return 0;
}

int main() {
    setlocale(LC_ALL, "");
    InitializeGDIPlus(); // 初始化 GDI+

    OCRRecongize("C:\\Users\\Administrator\\Pictures\\1.png");
    std::wcout << L"--- 正在查找 'Vector CANdb++ Editor' 窗口 ---" << std::endl;
    HWND vectorHwnd = NULL;

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        wchar_t title[256] = { 0 };
        int len = GetWindowTextW(hwnd, title, 256);
        if (len > 0 && wcsstr(title, L"Vector CANdb++ Editor") != nullptr) {
            std::wcout << L"模糊匹配找到: 句柄 0x" << std::hex << hwnd << L" | 标题: " << title << std::endl;
            *reinterpret_cast<HWND*>(lParam) = hwnd;
            return FALSE;
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&vectorHwnd));

    if (vectorHwnd != NULL) {
        std::wcout << L"\n--- 找到窗口! 句柄: 0x" << std::hex << vectorHwnd << L" ---" << std::endl;

        HMENU hMenu = GetMenu(vectorHwnd);
        if (hMenu != NULL) {
            std::wcout << L"\n--- 开始收集菜单项信息 ---" << std::endl;

            std::vector<std::wstring> initialParentPath;
            std::vector<MenuItemInfo> allMenuItems;

            CollectMenuItems(hMenu, initialParentPath, allMenuItems);

            std::wcout << L"共收集到 " << allMenuItems.size() << L" 个可点击的菜单项。" << std::endl;

            std::wstring targetTopLevelName = L"&File";
            std::wstring targetSubPrefix = L"Create Data";
            UINT targetCommandId = 0;
            std::wstring matchedSubName;
            int topLevelIndex = -1;
            int subItemIndex = -1;

            for (size_t itemIdx = 0; itemIdx < allMenuItems.size(); ++itemIdx) {
                const auto& item = allMenuItems[itemIdx];
                if (item.path.size() != 2) continue;
                if (item.path[0] != targetTopLevelName) continue;

                if (item.path[1].length() >= targetSubPrefix.length() &&
                    item.path[1].compare(0, targetSubPrefix.length(), targetSubPrefix) == 0) {

                    targetCommandId = item.commandId;
                    matchedSubName = item.path[1];

                    int count = GetMenuItemCount(hMenu);
                    for (int i = 0; i < count; ++i) {
                        wchar_t tempText[256] = { 0 };
                        if (GetMenuStringW(hMenu, i, tempText, 256, MF_BYPOSITION) > 0) {
                            if (std::wstring(tempText) == targetTopLevelName) {
                                topLevelIndex = i;
                                break;
                            }
                        }
                    }

                    if (topLevelIndex != -1) {
                        HMENU subMenu = GetSubMenu(hMenu, topLevelIndex);
                        if (subMenu) {
                            int subCount = GetMenuItemCount(subMenu);
                            for (int j = 0; j < subCount; ++j) {
                                wchar_t tempSubText[256] = { 0 };
                                if (GetMenuStringW(subMenu, j, tempSubText, 256, MF_BYPOSITION) > 0) {
                                    if (std::wstring(tempSubText).find(targetSubPrefix) == 0) { // Use find for prefix match
                                        subItemIndex = j;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    std::wcout << L"找到目标菜单项 '" << targetTopLevelName << L"' -> '" << matchedSubName << L"'!" << std::endl;
                    std::wcout << L"命令ID: 0x" << std::hex << targetCommandId << std::endl;
                    std::wcout << L"顶级菜单索引: " << topLevelIndex << L", 子菜单索引: " << subItemIndex << std::endl;
                    break;
                }
            }

            if (targetCommandId != 0 && topLevelIndex != -1 && subItemIndex != -1) {

                // --- 第二步：触发菜单展开 ---
                wchar_t topLevelText[256] = { 0 };
                GetMenuStringW(hMenu, topLevelIndex, topLevelText, 256, MF_BYPOSITION);
                wchar_t mnemonicChar = 0;
                for (wchar_t* p = topLevelText; *p; ++p) {
                    if (*p == L'&' && *(p + 1)) {
                        mnemonicChar = *(p + 1);
                        break;
                    }
                }

                if (mnemonicChar) {
                    std::wcout << L"模拟按下 Alt+" << mnemonicChar << L" 以展开菜单..." << std::endl;
                    // 尝试使用 PostMessage
                    PostMessageW(vectorHwnd, WM_SYSCOMMAND, SC_KEYMENU, (LPARAM)mnemonicChar);
                    Sleep(1000); // 给予更多时间
                    // 再次尝试，这次用 SendMessage 看看效果（注意：可能还是会卡）
                    // SendMessageW(vectorHwnd, WM_SYSCOMMAND, SC_KEYMENU, (LPARAM)mnemonicChar);
                    // Sleep(1000);
                }
                else {
                    std::wcout << L"警告: 无法找到顶级菜单的助记符。" << std::endl;
                }

                // --- 第三步：使用 PrintWindow 截图 ---
                std::wcout << L"--- 尝试使用 PrintWindow 截取窗口 ---" << std::endl;
                std::wstring pngPath = L"window_with_menu_gdiplus.png";
                if (!CaptureAndSaveWindowWithGDIPlus(vectorHwnd, pngPath.c_str())) {
                    std::wcout << L"❌ PrintWindow/GDI+ 截图失败。" << std::endl;
                }

                // --- 第四步：发送命令点击 ---
                std::wcout << L"\n--- 尝试发送 WM_COMMAND 消息以点击菜单项 ---" << std::endl;
                WPARAM wParam = MAKEWPARAM(targetCommandId, 0);
                LPARAM lParam = 0;
                PostMessageW(vectorHwnd, WM_COMMAND, wParam, lParam);

                std::wcout << L"WM_COMMAND 消息已发送，可能已触发 '" << matchedSubName << L"'。" << std::endl;

            }
            else {
                std::wcout << L"\n--- 未找到匹配的菜单项或无法确定其索引 'File' -> 以 '" << targetSubPrefix << L"' 开头 ---" << std::endl;
                std::wcout << L"以下是 'File' 菜单下的所有可点击子项:" << std::endl;
                for (const auto& item : allMenuItems) {
                    if (item.path.size() == 2 && item.path[0] == L"&File") {
                        wprintf(L"  '%ls' (ID: 0x%X)\n", item.path[1].c_str(), item.commandId);
                    }
                }
            }

        }
        else {
            std::wcout << L"窗口没有菜单或获取菜单失败。错误码: " << GetLastError() << std::endl;
        }

    }
    else {
        std::wcout << L"\n--- 未能找到 'Vector CANdb++ Editor' 窗口 ---" << std::endl;
    }

    ShutdownGDIPlus(); // 关闭 GDI+
    std::wcout << L"\n--- 程序结束 ---" << std::endl;
    system("pause");
    return 0;
}

// 你的其他函数 (CollectMenuItems, GetMenuItemScreenRect, SaveWindowRegionToBmp, OCRRecongize) 放在这里
// ... (其他函数) ...