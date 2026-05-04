// data_sources/czce_data_source.cpp
#include "czce_data_source.h"
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include "rapidjson/document.h"
using namespace rapidjson;

#pragma comment(lib, "winhttp.lib")

namespace { // 私有辅助函数
    std::string http_get(const std::wstring& host, const std::wstring& path) {
        // ... (复用 DCE 的 http_get 实现)
        std::string resp;
        HINTERNET hSession = WinHttpOpen(L"client/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
        if (!hSession) return resp;

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return resp;
        }

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, NULL, NULL, WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return resp;
        }

        BOOL bSend = WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);
        if (bSend) {
            WinHttpReceiveResponse(hRequest, NULL);
            char buf[4096];
            DWORD read = 0;
            while (WinHttpReadData(hRequest, buf, sizeof(buf), &read) && read > 0) {
                resp.append(buf, read);
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return resp;
    }
} // namespace

std::vector<TickData> CzceDataSource::fetchQuotes(const std::string& commodity) {
    std::vector<TickData> res;
    // 🔥 修复：正确的 host 和 path
    std::wstring host = L"www.czce.com.cn";
    std::wstring path = L"/api/quotes/contract?contract=" + std::wstring(commodity.begin(), commodity.end());

    std::string resp = http_get(host, path);
    if (resp.empty()) {
        std::cerr << "CZCE: HTTP GET failed for " << commodity << std::endl;
        return res;
    }

    Document doc;
    doc.Parse(resp.c_str());
    if (!doc.IsObject() || !doc.HasMember("data") || !doc["data"].IsArray()) {
        std::cerr << "CZCE: Invalid response format for " << commodity << std::endl;
        return res;
    }

    const Value& dataArray = doc["data"];
    for (SizeType i = 0; i < dataArray.Size(); i++) {
        const Value& item = dataArray[i];
        if (!item.IsObject()) continue;

        TickData q{};
        q.source = getName(); // 标记来源

        if (item.HasMember("instrumentId") && item["instrumentId"].IsString())
            q.instrument = item["instrumentId"].GetString();

        if (item.HasMember("last") && item["last"].IsDouble())
            q.last = item["last"].GetDouble();

        if (item.HasMember("open") && item["open"].IsDouble())
            q.open = item["open"].GetDouble();

        if (item.HasMember("high") && item["high"].IsDouble())
            q.high = item["high"].GetDouble();

        if (item.HasMember("low") && item["low"].IsDouble())
            q.low = item["low"].GetDouble();

        if (item.HasMember("bid1") && item["bid1"].IsDouble())
            q.bid1 = item["bid1"].GetDouble();

        if (item.HasMember("ask1") && item["ask1"].IsDouble())
            q.ask1 = item["ask1"].GetDouble();

        if (item.HasMember("volume") && item["volume"].IsInt64())
            q.volume = static_cast<long long>(item["volume"].GetInt64());

        if (item.HasMember("oi") && item["oi"].IsDouble())
            q.open_interest = static_cast<long long>(item["oi"].GetDouble());

        if (item.HasMember("time") && item["time"].IsString())
            q.time = item["time"].GetString();

        res.push_back(q);
    }
    return res;
}