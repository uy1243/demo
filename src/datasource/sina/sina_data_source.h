// data_sources/sina_data_source.h
#pragma once
#include "../market_types.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <mutex>

class SinaDataSource : public IDataSource {
public:
    std::string getName() const override { return "SINA"; }
    std::vector<TickData> fetchQuotes(const std::string& instrument) override;
    void start() override; // 启动订阅线程
    void stop() override;  // 停止订阅线程

    void subscribe(const std::string& instrument);

private:
    void run_loop();

private:
    std::atomic<bool> running_{ false };
    std::thread worker_thread_;
    std::unordered_set<std::string> subscriptions_;
    mutable std::mutex subs_mtx_;
};