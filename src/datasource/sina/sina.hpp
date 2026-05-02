#include <curl/curl.h>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>

// GBK → UTF-8（Windows）
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

size_t write(void* c, size_t s, size_t n, std::string* o) {
    o->append((char*)c, s * n);
    return s * n;
}

// 期货实时
struct FutTick {
    std::string code;
    std::string time;
    double open, high, low, price;
    long long volume;
};

FutTick download_fut_tick(const std::string& code) {
    CURL* curl = curl_easy_init();
    std::string r;
    std::string url = "http://hq.sinajs.cn/list=NF_" + code;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
    struct curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Referer: http://finance.sina.com.cn");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(hdrs);

    // 解析 var hq_str_NF_IF2509="..."
    size_t p = r.find("=\"");
    if (p == std::string::npos) return {};
    std::string data = r.substr(p + 2);
    data.pop_back(); // 去掉末尾 "

    // GBK→UTF8
    data = GBKToUTF8(data);

    FutTick t;
    t.code = code;
    sscanf_s(data.c_str(), "%[^,],%lf,%lf,%lf,%lf,%lld",
        &t.time[0], 20,
        &t.open, &t.high, &t.low, &t.price, &t.volume);
    return t;
}

// 期权实时
struct OptTick {
    std::string code;
    std::string time;
    double bid, ask, price;
    long long volume;
    long long open_interest;
};

OptTick download_opt_tick(const std::string& code) {
    CURL* curl = curl_easy_init();
    std::string r;
    std::string url = "https://hq.sinajs.cn/etag.php?list=P_OP_" + code;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
    struct curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Referer: http://finance.sina.com.cn");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(hdrs);

    size_t p = r.find("=\"");
    if (p == std::string::npos) return {};
    std::string data = r.substr(p + 2);
    data.pop_back();
    data = GBKToUTF8(data);

    OptTick t;
    t.code = code;
    // 状态,买价,卖价,现价,成交量,持仓量
    sscanf_s(data.c_str(), "%*[^,],%lf,%lf,%lf,%lld,%lld",
        &t.bid, &t.ask, &t.price, &t.volume, &t.open_interest);
    return t;
}

