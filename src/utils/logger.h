#pragma once
#include <cstdio>
#include <cstdarg>
#include <time.h>
#include <cstring>
#include <string>

#define LOG_INFO(...) log_write("INFO", __VA_ARGS__)
#define LOG_ERR(...) log_write("ERROR", __VA_ARGS__)

#ifdef _WIN32
#include <windows.h>
static std::string GbkToUtf8(const char* gbk_str) {
    if (!gbk_str) return "";
    int len = MultiByteToWideChar(CP_ACP, 0, gbk_str, -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len + 1];
    MultiByteToWideChar(CP_ACP, 0, gbk_str, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* utf8str = new char[len + 1];
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8str, len, NULL, NULL);
    std::string res = utf8str;
    delete[] wstr;
    delete[] utf8str;
    return res;
}
#endif

// 只留声明！！！
void log_write(const char* level, const char* format, ...);