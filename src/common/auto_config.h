#pragma once
#include <string>
#include <iostream>
#include <map>
#include <functional>
#include <fstream>

// 引入 RapidJSON
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/error/en.h"

using namespace rapidjson;

// ============================================================================
// 1. 配置管理器 (单例)
// ============================================================================
class ConfigManager {
public:
    using SetterFunc = std::function<void(const Value&)>;

    static ConfigManager& GetInstance() {
        static ConfigManager instance;
        return instance;
    }

    void Register(const std::string& key, SetterFunc setter) {
        setters_[key] = setter;
    }

    void LoadFromFile(const std::string& filename) {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            return;
        }

        IStreamWrapper isw(ifs);
        Document doc;
        if (doc.ParseStream(isw).HasParseError()) {
            std::cerr << "[Config] JSON 解析失败: " << GetParseError_En(doc.GetParseError()) << std::endl;
            return;
        }

        ApplyJson(doc);
    }

private:
    std::map<std::string, SetterFunc> setters_;

    void ApplyJson(const Document& doc) {
        if (!doc.IsObject()) return;

        for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
            std::string key = it->name.GetString();
            auto finder = setters_.find(key);
            if (finder != setters_.end()) {
                finder->second(it->value);
            }
        }
    }
};

// ============================================================================
// 2. 自动加载触发器
// ============================================================================
struct AutoConfigLoader {
    AutoConfigLoader(const std::string& filename) {
        ConfigManager::GetInstance().LoadFromFile(filename);
    }
};

// ============================================================================
// 3. 宏定义 (已修复 Bool 定义)
// ============================================================================

// 字符串
#define DEFINE_STRING(Name, DefaultVal, Desc) \
    std::string FLAG_##Name = DefaultVal; \
    struct Name##_Registrar { \
        Name##_Registrar() { \
            ConfigManager::GetInstance().Register(#Name, [](const Value& val) { \
                if (val.IsString()) { \
                    FLAG_##Name = std::string(val.GetString()); \
                } \
            }); \
        } \
    } Name##_registrar_instance; \
    AutoConfigLoader Name##_auto_loader("config.json");

// 布尔类型 (修复：显式调用 IsBool 和 GetBool)
#define DEFINE_BOOL(Name, DefaultVal, Desc) \
    bool FLAG_##Name = DefaultVal; \
    struct Name##_Registrar { \
        Name##_Registrar() { \
            ConfigManager::GetInstance().Register(#Name, [](const Value& val) { \
                if (val.IsBool()) { \
                    FLAG_##Name = val.GetBool(); \
                } \
            }); \
        } \
    } Name##_registrar_instance; \
    AutoConfigLoader Name##_auto_loader("config.json");

// 整数 (Int 方法存在，可以使用通用宏逻辑)
#define DEFINE_INT32(Name, DefaultVal, Desc) \
    int32_t FLAG_##Name = DefaultVal; \
    struct Name##_Registrar { \
        Name##_Registrar() { \
            ConfigManager::GetInstance().Register(#Name, [](const Value& val) { \
                if (val.IsInt()) { \
                    FLAG_##Name = val.GetInt(); \
                } \
            }); \
        } \
    } Name##_registrar_instance; \
    AutoConfigLoader Name##_auto_loader("config.json");

#define DEFINE_INT64(Name, DefaultVal, Desc) \
    int64_t FLAG_##Name = DefaultVal; \
    struct Name##_Registrar { \
        Name##_Registrar() { \
            ConfigManager::GetInstance().Register(#Name, [](const Value& val) { \
                if (val.IsInt()) { \
                    FLAG_##Name = val.GetInt(); \
                } \
            }); \
        } \
    } Name##_registrar_instance; \
    AutoConfigLoader Name##_auto_loader("config.json");

// 双精度浮点数
#define DEFINE_DOUBLE(Name, DefaultVal, Desc) \
    double FLAG_##Name = DefaultVal; \
    struct Name##_Registrar { \
        Name##_Registrar() { \
            ConfigManager::GetInstance().Register(#Name, [](const Value& val) { \
                if (val.IsDouble()) { \
                    FLAG_##Name = val.GetDouble(); \
                } \
            }); \
        } \
    } Name##_registrar_instance; \
    AutoConfigLoader Name##_auto_loader("config.json");