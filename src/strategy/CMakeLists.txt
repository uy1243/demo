set(RapidJSON_INCLUDE_DIR "C:/Users/naiyu/source/repos/uy1243/demo/vcpkg_installed/vcpkg/pkgs/rapidjson_x64-windows/include")
# 🔥 强制写死路径（绝对不出错）
set(MYSQL_PATH "C:/Program Files/MySQL/MySQL Connector C++ 9.7")
add_definitions(-DCURL_STATICLIB)
# 🔥 头文件路径（解决 C1083 错误）
include_directories(
${RapidJSON_INCLUDE_DIR})

add_executable(stat
    main.cpp
    
    ../datasource/market_data.cpp
    "../analyze/market_analyzer.cpp"
 "kline.h" "kline.cpp" "strategy2.cpp")

target_link_libraries(stat PRIVATE
    winhttp
)