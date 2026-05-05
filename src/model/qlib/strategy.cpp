// strategy/my_strategy.h/cpp
class MyStrategy {
    // ...
    QlibDataSource* qlib_source_; // 注入或获取 Qlib 数据源
    // ...
public:
    void onMarketUpdate(const MarketUpdateEvent& event) {
        // 获取当前市场数据
        auto& cache = MarketService::Instance().getCache(); // 假设有单例或可访问的cache
        std::string current_inst = "m2509"; // 当前策略关注的合约
        auto current_tick = cache.getLatestTick(current_inst);

        // 查询 Qlib 提供的最新信号
        auto factors = qlib_source_->getFactorData(current_inst, current_tick.time, current_tick.time); // 查询当前时刻的信号
        if (!factors.empty()) {
            auto latest_factor = factors.back(); // 获取最新记录
            if (latest_factor.signal == "BUY") {
                // 执行买入逻辑
                placeOrder(OrderRequest{ ... });
            }
            else if (latest_factor.signal == "SELL") {
                // 执行卖出逻辑
                cancelOrder(...);
            }
            // 也可以直接使用 latest_factor.alpha001, latest_factor.volatility_5d 等数值进行判断
        }
    }
};