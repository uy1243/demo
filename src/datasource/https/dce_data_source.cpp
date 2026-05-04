// data_sources/dce_data_source.cpp
#include "dce_data_source.h"
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include "rapidjson/document.h"
using namespace rapidjson;

#pragma comment(lib, "winhttp.lib")

namespace { // 私有辅助函数
    std::string http_get(const std::wstring& host, const std::wstring& path) {
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

std::vector<TickData> DceDataSource::fetchQuotes(const std::string& commodity) {
    std::vector<TickData> res;
    std::wstring host = L"www.dce.com.cn";
    std::wstring path = L"/portal/category/market/quote.do?code=" + std::wstring(commodity.begin(), commodity.end());

    std::string resp = http_get(host, path);
    if (resp.empty()) {
        std::cerr << "DCE: HTTP GET failed for " << commodity << std::endl;
        return res;
    }

    Document doc;
    doc.Parse(resp.c_str());
    if (!doc.IsObject() || !doc.HasMember("data") || !doc["data"].HasMember("tables")) {
        std::cerr << "DCE: Invalid response format for " << commodity << std::endl;
        return res;
    }

    const Value& tables = doc["data"]["tables"];
    if (!tables.IsArray()) return res;

    for (SizeType i = 0; i < tables.Size(); i++) {
        const Value& item = tables[i];
        if (!item.IsObject()) continue;

        TickData q{};
        q.source = getName(); // 标记来源

        if (item.HasMember("instrumentId") && item["instrumentId"].IsString())
            q.instrument = item["instrumentId"].GetString();

        if (item.HasMember("lastPrice") && item["lastPrice"].IsDouble())
            q.last = item["lastPrice"].GetDouble();

        if (item.HasMember("openPrice") && item["openPrice"].IsDouble())
            q.open = item["openPrice"].GetDouble();

        if (item.HasMember("highPrice") && item["highPrice"].IsDouble())
            q.high = item["highPrice"].GetDouble();

        if (item.HasMember("lowPrice") && item["lowPrice"].IsDouble())
            q.low = item["lowPrice"].GetDouble();

        if (item.HasMember("bidPrice1") && item["bidPrice1"].IsDouble())
            q.bid1 = item["bidPrice1"].GetDouble();

        if (item.HasMember("askPrice1") && item["askPrice1"].IsDouble())
            q.ask1 = item["askPrice1"].GetDouble();

        if (item.HasMember("volume") && item["volume"].IsInt64())
            q.volume = static_cast<long long>(item["volume"].GetInt64());

        if (item.HasMember("openInterest") && item["openInterest"].IsDouble())
            q.open_interest = static_cast<long long>(item["openInterest"].GetDouble());

        if (item.HasMember("updateTime") && item["updateTime"].IsString())
            q.time = item["updateTime"].GetString();

        res.push_back(q);
    }
    return res;
}