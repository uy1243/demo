#include "config.h"
#include <cstdlib>

Config& Config::Instance() {
    static Config ins;
    return ins;
}
bool Config::load(const std::string& file) { return true; }
std::string Config::getStr(const std::string& key) { return ""; }
int Config::getInt(const std::string& key) { return 0; }