add_executable(main_test
    src/main.cpp
    src/market_data.cpp
    src/analysis.cpp
    src/account.cpp
    src/account_view.cpp
)

target_link_libraries(main_test PRIVATE winhttp)