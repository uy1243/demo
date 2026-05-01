#include "sina.hpp"
#include <curl/curl.h>
#include <sstream>

size_t write(void* c, size_t s, size_t n, std::string* o) {
    o->append((char*)c, s * n);
    return s * n;
}

std::vector<Bar> download_sina_history(const std::string& code) {
    CURL* curl = curl_easy_init();
    std::string r;
    std::string url = "https://money.finance.sina.com.cn/pc/history/kline.php?symbol=" + code + "&period=1&num=1000";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    std::vector<Bar> res;
    std::stringstream ss(r);
    std::string line;
    while (std::getline(ss, line)) {
        Bar b;
        b.instrument = code;
        sscanf_s(line.c_str(), "%s %lf,%lf,%lf,%lf,%d",
            b.time.data(), 20,
            &b.open, &b.high, &b.low, &b.close, &b.volume);
        res.push_back(b);
    }
    return res;
}

void save_csv(const std::vector<Bar>& bars, const std::string& path) {
    std::ofstream f(path);
    f << "instrument,time,open,high,low,close,volume\n";
    for (auto& b : bars) {
        f << b.instrument << "," << b.time << "," << b.open << "," << b.high << "," << b.low << "," << b.close << "," << b.volume << "\n";
    }
}