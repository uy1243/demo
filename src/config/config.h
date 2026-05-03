#pragma once
#include <string>

class Config {
public:
    static Config& Instance();
    bool load(const std::string& file);
    std::string getStr(const std::string& key);
    int getInt(const std::string& key);
private:
    Config() = default;
};