#include <iostream>
#include <vector>
#include "sina.hpp"
#include "utils/logger.h"

// 必须加上，否则 save_csv 找不到 ofstream
#include <fstream>

// 你的 Bar 结构必须定义（我直接给你补上）
struct Bar {
    std::string instrument;
    std::string time;
    double open = 0;
    double high = 0;
    double low = 0;
    double close = 0;
    int volume = 0;
};
