// data_sources/sina_data_source.cpp
#include "sina_data_source.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <cstring>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
std::string GBKToUTF8(const std::string& gbk) {
    int len = MultiByteToWideChar(936, 0, gbk.c_str(), -1, nullptr, 0);
    std::wstring wstr(len, L'\0');
    MultiByteToWideChar(936, 0, gbk.c_str(), -1, &wstr[0], len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8[0], len, nullptr, nullptr);
    return utf8;
}
#else
std::string GBKToUTF8(const std::string& s) { return s; }
#endif

namespace { // 私有辅助函数
    size_t write(void* c, size_t s, size_t n, std::string* o) {
        o->append((char*)c, s * n);
        return s * n;
    }

    TickData parse_fut_tick(const std::string& raw_line, const std::string& code) {
        TickData t{};
        t.source = "SINA";
        t.instrument = code;
        // 示例解析 (格式可能变化，需根据实际API调整)
        // 假设格式为: "time,open,high,low,price,volume,..."
        std::istringstream iss(raw_line);
        std::string time_str;
        if (std::getline(iss, time_str, ',') &&
            (iss >> t.open >> t.high >> t.low >> t.last >> t.volume)) {
            t.time = time_str;
            t.bid1 = t.last - 0.5; // 示例计算
            t.ask1 = t.last + 0.5;
        }
        return t;
    }
} // namespace

std::vector<TickData> SinaDataSource::fetchQuotes(const std::string& instrument) {
    std::vector<TickData> res;
    CURL* curl = curl_easy_init();
    if (!curl) return res;

    std::string response;
    std::string url = "http://hq.sinajs.cn/list=NF_" + instrument;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Referer: http://finance.sina.com.cn");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res_code = curl_easy_perform(curl);
    if (res_code == CURLE_OK) {
        size_t pos = response.find("=\"");
        if (pos != std::string::npos) {
            std::string data = response.substr(pos + 2);
            if (!data.empty() && data.back() == '"') data.pop_back();
            data = GBKToUTF8(data);
            res.push_back(parse_fut_tick(data, instrument));
        }
    }
    else {
        std::cerr << "SINA: Failed to fetch " << instrument << ", error: " << curl_easy_strerror(res_code) << std::endl;
    }

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return res;
}

void SinaDataSource::subscribe(const std::string& instrument) {
    std::lock_guard<std::mutex> lock(subs_mtx_);
    subscriptions_.insert(instrument);
}

void SinaDataSource::start() {
    if (running_.exchange(true)) return; // 防止重复启动
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
            // 这里需要一个回调或事件机制来通知 MarketCache 更新
            // 为了简化，暂时打印
            if (!ticks.empty()) {
                std::cout << "SINA Update: " << ticks[0].instrument << " @ " << ticks[0].last << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 模拟刷新间隔
    }
}