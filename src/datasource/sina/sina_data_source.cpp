#include "sina_data_source.h"
#include <sstream>
#include <iostream>
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

using namespace std;

std::string GBKToUTF8(const std::string& gbk)
{
    // 【修复】GBK转UTF8 修复空字符串崩溃问题
    if (gbk.empty()) return "";

    int len = MultiByteToWideChar(936, 0, gbk.c_str(), -1, nullptr, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(936, 0, gbk.c_str(), -1, &wstr[0], len);

    len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);

    return utf8;
}

namespace {

    TickData parse_fut_tick(const std::string& data, const std::string& instrument)
    {
        TickData t{};
        t.source = "SINA";
        t.instrument = instrument;

        vector<string> fields;
        stringstream ss(data);
        string field;

        while (getline(ss, field, ',')) {
            fields.push_back(field);
        }

        // ===================== 【核心修复】新浪 M0 真实字段顺序 =====================
        if (fields.size() >= 17) {
            try {
                t.open = atof(fields[2].c_str());      // 开盘
                t.high = atof(fields[3].c_str());      // 最高
                t.low = atof(fields[4].c_str());       // 最低
                t.last = atof(fields[8].c_str());      // 最新价 ✅正确位置
                t.bid1 = atof(fields[6].c_str());      // 买一
                t.ask1 = atof(fields[7].c_str());      // 卖一
                t.volume = atoll(fields[13].c_str());  // 成交量
                t.open_interest = atoll(fields[12].c_str());
                t.time = fields[16];                   // 日期 ✅正确位置
            }
            catch (...) {
                // 不崩溃
            }
        }
        return t;
    }

    bool WinHttpGet(
        const std::wstring& host,
        const std::wstring& path,
        std::string& out_response)
    {
        HINTERNET hSession = WinHttpOpen(
            L"Mozilla/5.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0);

        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect, L"GET", path.c_str(), nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        const wchar_t* referer = L"Referer: https://finance.sina.com.cn/";
        WinHttpAddRequestHeaders(hRequest, referer, (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

        BOOL sent = WinHttpSendRequest(hRequest, WINHTTP_NO_REQUEST_DATA, 0, nullptr, 0, 0, 0);
        if (!sent) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        BOOL recv = WinHttpReceiveResponse(hRequest, nullptr);
        if (!recv) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        out_response.clear();
        char buf[4096];
        DWORD read = 0;
        while (WinHttpReadData(hRequest, buf, sizeof(buf), &read) && read > 0) {
            out_response.append(buf, read);
            read = 0;
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return true;
    }
}

vector<TickData> SinaDataSource::fetchQuotes(const string& instrument)
{
    cout << "[SINA] 拉取: " << instrument << endl;
    vector<TickData> res;

    wstring host = L"hq.sinajs.cn";
    wstring path = L"/list=nf_" + wstring(instrument.begin(), instrument.end());

    string response;
    WinHttpGet(host, path, response);
	std::cout << " [SINA] 原始响应: " << response << std::endl;

    size_t pos1 = response.find("=\"");
    size_t pos2 = response.find("\";", pos1);

    if (pos1 != string::npos && pos2 != string::npos) {
        string data = response.substr(pos1 + 2, pos2 - pos1 - 2);
        data = GBKToUTF8(data);

        TickData tick = parse_fut_tick(data, instrument);
        if (tick.last > 0) {
            res.push_back(tick);
        }
    }
    return res;
}

std::multimap<std::string, TickData> SinaDataSource::fetchHistoricalData(
    const std::string& instrument,
    const std::string& start_time,
    const std::string& end_time) {
    return {};
}