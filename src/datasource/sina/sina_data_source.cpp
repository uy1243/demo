#include "sina_data_source.h"
#include <sstream>
#include <iostream>
#include <windows.h>
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

std::string GBKToUTF8(const std::string& gbk) {
    int len = MultiByteToWideChar(936, 0, gbk.c_str(), -1, nullptr, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(936, 0, gbk.c_str(), -1, &wstr[0], len);

    len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    return utf8;
}

namespace {
    size_t write(void* c, size_t s, size_t n, std::string* o) {
        o->append((char*)c, s * n);
        return s * n;
    }

    TickData parse_fut_tick(const std::string& raw_line, const std::string& code) {
        TickData t{};
        t.source = "SINA";
        t.instrument = code;

        std::istringstream iss(raw_line);
        std::string time_str;
        if (std::getline(iss, time_str, ',') &&
            (iss >> t.open >> t.high >> t.low >> t.last >> t.volume)) {
            t.time = time_str;
            t.bid1 = t.last - 0.5;
            t.ask1 = t.last + 0.5;
        }
        return t;
    }

    // WinHttp 实现 GET 请求
    bool WinHttpGet(
        const std::wstring& host,
        const std::wstring& path,
        const std::wstring& referer,
        std::string& out_response
    ) {
        HINTERNET hSession = WinHttpOpen(
            L"Mozilla/5.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS, 0
        );
        if (!hSession) return false;

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return false; }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0
        );
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

        // 添加 Referer
        WinHttpAddRequestHeaders(hRequest, referer.c_str(), (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);

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

// 核心：抓取行情（WinHttp 版）
std::vector<TickData> SinaDataSource::fetchQuotes(const std::string& instrument) {
    std::vector<TickData> res;

    std::wstring host = L"hq.sinajs.cn";
    std::wstring path = L"/list=NF_" + std::wstring(instrument.begin(), instrument.end());
    std::wstring referer = L"Referer: http://finance.sina.com.cn";

    std::string response;
    if (!WinHttpGet(host, path, referer, response)) {
        std::cerr << "SINA: WinHttp 请求失败 " << instrument << std::endl;
        return res;
    }

    size_t pos = response.find("=\"");
    if (pos != std::string::npos) {
        std::string data = response.substr(pos + 2);
        if (!data.empty() && data.back() == '"') data.pop_back();
        data = GBKToUTF8(data);
        res.push_back(parse_fut_tick(data, instrument));
    }

    return res;
}

// 以下完全不变，保持原有接口
void SinaDataSource::subscribe(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(subs_mtx_);
    subscriptions_.insert(instrument);
}

void SinaDataSource::start() {
    if (running_.exchange(true)) return;
    worker_thread_ = std::thread(&SinaDataSource::run_loop, this);
}

void SinaDataSource::stop() {
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void SinaDataSource::run_loop() {
    while (running_) {
        std::unordered_set<std::string> current_subs;
        {
            std::lock_guard<std::mutex> lock(subs_mtx_);
            current_subs = subscriptions_;
        }

        for (const auto& inst : current_subs) {
            auto ticks = fetchQuotes(inst);
            if (!ticks.empty()) {
                std::cout << "SINA Update: " << ticks[0].instrument << " @ " << ticks[0].last << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}