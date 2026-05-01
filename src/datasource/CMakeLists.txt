cmake_minimum_required(VERSION 3.15)
project(mysql_test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 🔥 强制写死路径（绝对不出错）
set(MYSQL_PATH "C:/Program Files/MySQL/MySQL Connector C++ 9.7")

# 🔥 头文件路径（解决 C1083 错误）
include_directories(${MYSQL_PATH}/include)

# 🔥 库路径
link_directories(${MYSQL_PATH}/lib64
                ${MYSQL_PATH}/lib64/vs14/)

# 打印看看路径对不对（你可以看到正确路径了）
message("MySQL 头文件路径：${MYSQL_PATH}/include")

# 可执行文件
add_executable(mysql_test
    persistent/add.cpp
)

# 链接正确的库
target_link_libraries(mysql_test PRIVATE
    mysqlcppconn
    mysqlcppconnx
)

# 自动复制 DLL
if(MSVC)
    set(MYSQL_BIN_DIR "${MYSQL_PATH}/lib64")

    set(OUT_DIR "$<TARGET_FILE_DIR:mysql_test>")

    add_custom_command(TARGET mysql_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MYSQL_BIN_DIR}/mysqlcppconn-10-vs14.dll" ${OUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MYSQL_BIN_DIR}/mysqlcppconnx-2-vs14.dll" ${OUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MYSQL_BIN_DIR}/libcrypto-3-x64.dll" ${OUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MYSQL_BIN_DIR}/libssl-3-x64.dll" ${OUT_DIR}
    )
endif()