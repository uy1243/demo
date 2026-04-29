#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <curl/curl.h>
#include <boost/thread/mutex.hpp>

// 行情结构体
struct MarketData {
    std::string code;       // 合约代码
    double last_price;      // 最新价
    double high;             // 最高价
    double low;              // 最低价
    int volume;              // 成交量
    std::string timestamp;   // 时间
};

// 内存存储（线程安全）
std::unordered_map<std::string, MarketData> g_market_data;
boost::mutex g_mutex;

// CURL 回调
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    }
    catch (std::bad_alloc& e) {
        return 0;
    }
    return newLength;
}

// GBK→UTF-8 解码（简化版，可用iconv）
std::string GBKtoUTF8(const std::string& gbk) {
    // 实际项目用iconv库转换，此处省略
    return gbk;
}

// 获取新浪行情
bool fetchSinaQuote(const std::string& url, const std::string& code) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_REFERER, "http://finance.sina.com.cn"); // 必须Referer
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return false;

    // 解析response（截取引号内数据）
    size_t start = response.find("\"");
    size_t end = response.find("\"", start + 1);
    if (start == std::string::npos || end == std::string::npos) return false;

    std::string data = GBKtoUTF8(response.substr(start + 1, end - start - 1));
    std::vector<std::string> fields;
    // 分割逗号（简化版）
    size_t pos = 0;
    while ((pos = data.find(",")) != std::string::npos) {
        fields.push_back(data.substr(0, pos));
        data.erase(0, pos + 1);
    }

    // 存入内存
    boost::lock_guard<boost::mutex> lock(g_mutex);
    g_market_data[code] = {
        code,
        std::stod(fields[2]),
        std::stod(fields[3]),
        std::stod(fields[4]),
        std::stoi(fields[7]),
        fields[9]
    };
    return true;
}

int fetch() {
    // 需获取的合约
    std::vector<std::string> futures = { "nf_rb2510", "nf_ma2509", "nf_cu2509" };
    std::vector<std::string> options = { "P_OP_io2012P4000" };

    // 轮询（每秒一次）
    while (true) {
        for (const auto& code : futures) {
            fetchSinaQuote("https://hq.sinajs.cn/list=" + code, code);
        }
        for (const auto& code : options) {
            fetchSinaQuote("https://hq.sinajs.cn/etag.php?list=" + code, code);
        }

        // 打印内存数据
        boost::lock_guard<boost::mutex> lock(g_mutex);
        for (const auto& pair : g_market_data) {
            std::cout << pair.first << ": " << pair.second.last_price
                << " (成交量:" << pair.second.volume << ")" << std::endl;
        }
        sleep(1);
    }
    return 0;
}