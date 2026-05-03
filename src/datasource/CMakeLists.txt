cmake_minimum_required(VERSION 3.15)
project(mysql_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL CONFIG REQUIRED)
set(RapidJSON_INCLUDE_DIR "C:/Users/naiyu/source/repos/uy1243/demo/vcpkg_installed/vcpkg/pkgs/rapidjson_x64-windows/include")
# 🔥 强制写死路径（绝对不出错）
set(MYSQL_PATH "C:/Program Files/MySQL/MySQL Connector C++ 9.7")
add_definitions(-DCURL_STATICLIB)
# 🔥 头文件路径（解决 C1083 错误）
include_directories(${MYSQL_PATH}/include
${RapidJSON_INCLUDE_DIR})

# 🔥 库路径
link_directories(${MYSQL_PATH}/lib64
                ${MYSQL_PATH}/lib64/vs14/)

# 可执行文件
add_library(mysql_test
    persistent/add.cpp
    
 "ctp/ctp_test.cpp" "ctp/main.cpp")
# 链接正确的库
target_link_libraries(mysql_test PRIVATE
    mysqlcppconn
    mysqlcppconnx
)

add_library(sina_test 
"ctp/ctp_test.cpp"
    
  "ctp/main.cpp")

 add_executable(main_test 
    
    market_data.cpp
 "ctp/ctp_test.cpp" "ctp/main.cpp")

target_link_libraries(sina_test PRIVATE CURL::libcurl)

# 自动复制 DLL
if(MSVC)
    set(MYSQL_BIN_DIR "${MYSQL_PATH}/lib64")

    set(OUT_DIR "$<TARGET_FILE_DIR:mysql_test>")

    add_custom_command(TARGET mysql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MYSQL_BIN_DIR}/mysqlcppconn-10-vs14.dll" ${OUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MYSQL_BIN_DIR}/mysqlcppconnx-2-vs14.dll" ${OUT_DIR}
    )
endif()