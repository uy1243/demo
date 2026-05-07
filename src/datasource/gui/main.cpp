#include <iostream>
#include <windows.h>
#include <io.h> 
#include <fcntl.h> 
#include <cwchar> 
#include <clocale> 
#include <string> // For std::wstring

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

int main() {
    setlocale(LC_ALL, "");

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

            // --- 修改：根据实际输出调整前缀 ---
            // 输出显示是 "Create Data&base... Ctrl+N"，所以前缀应该是 "Create Data&base" 或更短的 "Create Data"
            std::wstring targetTopLevelName = L"File";
            std::wstring targetSubPrefix = L"Create Data"; // 使用更短的前缀以提高容错性
            UINT targetCommandId = 0;
            std::wstring matchedSubName;

            for (const auto& item : allMenuItems) {
                if (item.path.size() != 2) continue;
                if (item.path[0] != targetTopLevelName) continue;

                // 检查子菜单名称是否以 "Create Data" 开头
                if (item.path[1].length() >= targetSubPrefix.length() &&
                    item.path[1].compare(0, targetSubPrefix.length(), targetSubPrefix) == 0) {
                    targetCommandId = item.commandId;
                    matchedSubName = item.path[1];
                    std::wcout << L"找到目标菜单项 '" << targetTopLevelName << L"' -> '" << matchedSubName << L"'!" << std::endl;
                    std::wcout << L"命令ID: 0x" << std::hex << targetCommandId << std::endl;
                    break;
                }
            }

            if (targetCommandId != 0) {
                std::wcout << L"\n--- 尝试发送 WM_COMMAND 消息以点击菜单项 ---" << std::endl;

                WPARAM wParam = MAKEWPARAM(targetCommandId, 0);
                LPARAM lParam = 0;

                LRESULT result = SendMessageW(vectorHwnd, WM_COMMAND, wParam, lParam);

                if (result == 0) {
                    std::wcout << L"WM_COMMAND 消息发送成功，可能已触发 '" << matchedSubName << L"'。" << std::endl;
                }
                else {
                    std::wcout << L"WM_COMMAND 消息发送完成，返回值: " << result << L". (注意：返回值不一定代表成功/失败)" << std::endl;
                }
            }
            else {
                std::wcout << L"\n--- 未找到匹配的菜单项 'File' -> 以 '" << targetSubPrefix << L"' 开头 ---" << std::endl;
                std::wcout << L"以下是 'File' 菜单下的所有可点击子项:" << std::endl;
                for (const auto& item : allMenuItems) {
                    if (item.path.size() == 2 && item.path[0] == L"File") {
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