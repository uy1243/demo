#include <Windows.h>
#include <string>

using namespace std;

// 查找主窗口
HWND FindMainWindow(const wchar_t* windowTitle)
{
    return FindWindowW(NULL, windowTitle);
}

// 查找子控件（类名 / 标题）
HWND FindChildControl(HWND parent, const wchar_t* className, const wchar_t* caption)
{
    HWND child = NULL;
    while (true)
    {
        child = FindWindowExW(parent, child, className, caption);
        if (!child) break;
        return child;
    }
    return NULL;
}

// 发送文本到输入框（PostMessage）
void PostEditText(HWND hEdit, const wstring& text)
{
    for (wchar_t c : text)
    {
        PostMessageW(hEdit, WM_CHAR, c, 0);
        Sleep(10); // 稳定输入
    }
}

// 点击按钮（PostMessage）
void PostButtonClick(HWND hBtn)
{
    PostMessageW(hBtn, BM_CLICK, 0, 0);
}

// ====================== 你只需要改这里 ======================
const wchar_t* MAIN_WINDOW = L"你的窗口标题";
const wchar_t* INPUT_TEXT = L"你要输入的内容";
const wchar_t* BUTTON_CAPTION = L"按钮文字，例如：确定";
// ===========================================================

int main()
{
    HWND hMain = FindMainWindow(MAIN_WINDOW);
    if (!hMain)
    {
        MessageBoxW(NULL, L"找不到窗口", NULL, 0);
        return 0;
    }

    // 查找输入框（类名 Edit 99% 软件通用）
    HWND hEdit = FindChildControl(hMain, L"Edit", NULL);
    if (hEdit)
    {
        PostEditText(hEdit, INPUT_TEXT);
    }

    // 查找按钮
    HWND hBtn = FindChildControl(hMain, L"Button", BUTTON_CAPTION);
    if (hBtn)
    {
        Sleep(200);
        PostButtonClick(hBtn);
    }

    return 0;
}