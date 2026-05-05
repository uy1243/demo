// backtesting/backtest_engine.cpp
#include "backtest_engine.h"
#include "../strategy/event_system.h"
#include "../common/types.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <map>

// ==================== SimulatedTrader 实现 ====================

BacktestEngine::SimulatedTrader::SimulatedTrader(BacktestEngine* engine, EventSystem* event_system)
    : engine_(engine), event_system_(event_system) {
    commission_rate_ = engine_->commission_rate_;
    slippage_ = engine_->slippage_;
}

// 实现 initialize (目前可能不需要做特殊逻辑，或者保存 event_system)
void BacktestEngine::SimulatedTrader::initialize(EventSystem* event_system) {
    // 如果需要保存 event_system 指针，可以在这里赋值
     event_system_ = event_system; 
    // (注意：构造函数已经传入了 event_system，通常不需要在这里重复赋值，除非有特殊逻辑)
    // 如果不需要逻辑，保持空实现即可
}

OrderResponse BacktestEngine::SimulatedTrader::placeOrder(const OrderRequest& req) {
    OrderResponse resp;
    resp.local_order_ref = req.custom_order_ref;

    // 获取当前行情
    auto& cache = engine_->market_service_->getCache();
    TickData current_tick;
    if (!cache.get(req.instrument, current_tick)) {
        resp.status = OrderStatus::REJECTED;
        resp.error_msg = "No market data available";
        return resp;
    }

    // 模拟滑点
    double fill_price = req.price;
    if (slippage_ > 0) {
        if (req.direction == Direction::LONG) {
            fill_price = current_tick.ask1 > 0 ? current_tick.ask1 : current_tick.last * (1 + slippage_);
        }
        else {
            fill_price = current_tick.bid1 > 0 ? current_tick.bid1 : current_tick.last * (1 - slippage_);
        }
    }

    // 模拟成交（假设立即全部成交）
    resp.status = OrderStatus::FILLED;
    resp.exchange_order_id = "SIM_" + req.custom_order_ref;

    // 计算手续费
    double fee = fill_price * req.volume * commission_rate_;

    // 发布成交事件
    OrderUpdateEvent update_event(
        req.custom_order_ref,
        resp.exchange_order_id,
        OrderStatus::FILLED,
        req.volume,
        fill_price,
        ""
    );

    if (event_system_) {
        event_system_->publish(update_event);
    }

    // 记录交易
    engine_->result_.total_trades++;
    engine_->result_.total_fee += fee;

    return resp;
}

CancelResponse BacktestEngine::SimulatedTrader::cancelOrder(const CancelRequest& req) {
    CancelResponse resp;
    resp.local_order_ref = req.order_id;
    resp.success = true;
    return resp;
}

// ==================== BacktestEngine 实现 ====================

BacktestEngine::BacktestEngine(EventSystem* event_system, MarketService* market_service)
    : event_system_(event_system), market_service_(market_service) {
}

void BacktestEngine::stop() {
    // 如果正在运行，标记为停止
    if (running_.load()) {
        running_ = false;
        std::cout << "[BacktestEngine] 停止回测引擎..." << std::endl;

        // 这里可以添加额外的清理逻辑，例如：
        // 1. 等待工作线程结束
        // 2. 释放数据源资源
        // 3. 保存当前状态
    }
}

BacktestEngine::~BacktestEngine() {
    stop();
}

// 修复：参数类型改为 IDataSource
void BacktestEngine::addDataSource(std::unique_ptr<IDataSource> source) {
    data_sources_.push_back(std::move(source));
    std::cout << "[BacktestEngine] 添加数据源: " << data_sources_.back()->getName() << std::endl;
}

void BacktestEngine::setBacktestPeriod(const std::string& start_date, const std::string& end_date) {
    start_date_ = start_date;
    end_date_ = end_date;
}

void BacktestEngine::setInitialCapital(double capital) {
    initial_capital_ = capital;
}

void BacktestEngine::setCommissionRate(double rate) {
    commission_rate_ = rate;
}

void BacktestEngine::setSlippage(double slippage) {
    slippage_ = slippage;
}

void BacktestEngine::setInstruments(const std::vector<std::string>& instruments) {
    instruments_ = instruments;
}

void BacktestEngine::run() {
    if (running_.exchange(true)) {
        std::cout << "[BacktestEngine] Already running." << std::endl;
        return;
    }

    std::cout << "==========================================" << std::endl;
    std::cout << "        回测引擎启动" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "初始资金: " << std::fixed << std::setprecision(2) << initial_capital_ << " 元" << std::endl;
    std::cout << "回测周期: " << start_date_ << " 至 " << end_date_ << std::endl;
    std::cout << "手续费率: " << commission_rate_ * 100 << "%" << std::endl;
    std::cout << "滑点: " << slippage_ * 100 << "%" << std::endl;
    std::cout << "回测合约: ";
    for (const auto& inst : instruments_) {
        std::cout << inst << " ";
    }
    std::cout << std::endl;
    std::cout << "==========================================" << std::endl;

    // 初始化回测结果
    result_ = BacktestResult();
    result_.initial_capital = initial_capital_;

    // 创建模拟交易接口
    auto simulated_trader = std::make_shared<SimulatedTrader>(this, event_system_);

    // 初始化账户（使用模拟交易接口）
    auto& account = Account::Instance();
    account.initialize(event_system_, simulated_trader.get());

    // 初始化策略
    auto& strategy = Strategy::Instance();
    strategy.initialize(event_system_);

    // 执行回测
    simulateTicks();

    // 计算指标
    calculateMetrics();

    running_ = false;

    std::cout << "\n==========================================" << std::endl;
    std::cout << "        回测完成" << std::endl;
    std::cout << "==========================================" << std::endl;
}

void BacktestEngine::simulateTicks() {
    // 1. 从所有数据源获取历史数据，按时间排序
    std::priority_queue<TickData, std::vector<TickData>,
        std::function<bool(const TickData&, const TickData&)>>
        tick_queue([](const TickData& a, const TickData& b) {
        return a.time > b.time; // 最小堆，时间早的在顶部
            });

    // 记录每个时间点的权益
    std::map<std::string, double> equity_at_time;

    for (auto& source : data_sources_) {
        for (const auto& inst : instruments_) {
            std::cout << "正在加载 " << inst << " 的历史数据..." << std::endl;
            auto hist_data = source->fetchHistoricalData(inst, start_date_, end_date_);
            std::cout << "  加载了 " << hist_data.size() << " 条数据" << std::endl;

            for (const auto& [time, tick] : hist_data) {
                tick_queue.push(tick);
            }
        }
    }

    std::cout << "\n开始回测模拟..." << std::endl;
    std::cout << "总数据点: " << tick_queue.size() << std::endl;

    int processed = 0;
    int total = tick_queue.size();
    int last_percent = 0;

    // 2. 逐个推送 tick
    while (running_ && !tick_queue.empty()) {
        if (paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        TickData current_tick = tick_queue.top();
        tick_queue.pop();

        // 更新 MarketService 的缓存
        market_service_->getCache().update(current_tick);

        // 发布市场更新事件，触发策略
        event_system_->publish(MarketUpdateEvent{});

        // 记录当前权益
        auto& account = Account::Instance();
        auto fund = account.getFundInfo();
        equity_at_time[current_tick.time] = fund.total_asset;
        result_.equity_curve.push_back(fund.total_asset);
        result_.trade_dates.push_back(current_tick.time);

        // 计算回撤
        if (result_.equity_curve.size() > 1) {
            double peak = *std::max_element(result_.equity_curve.begin(), result_.equity_curve.end());
            double current_drawdown = (peak - fund.total_asset) / peak * 100;
            result_.drawdown_curve.push_back(current_drawdown);
        }

        // 进度显示
        processed++;
        int current_percent = (processed * 100) / total;
        if (current_percent > last_percent) {
            last_percent = current_percent;
            std::cout << "\r回测进度: " << current_percent << "% ("
                << processed << "/" << total << ")" << std::flush;
        }
    }

    std::cout << std::endl;

    // 获取最终结果
    auto& account = Account::Instance();
    auto final_fund = account.getFundInfo();
    result_.final_capital = final_fund.total_asset;
    result_.total_pnl = final_fund.position_pnl;
    result_.total_fee = final_fund.fee;
}

void BacktestEngine::calculateMetrics() {
    // 总收益率
    result_.total_return = (result_.final_capital - result_.initial_capital) / result_.initial_capital * 100;

    // 年化收益率（假设一年250个交易日）
    int trading_days = result_.equity_curve.size();
    if (trading_days > 0) {
        double years = trading_days / 250.0;
        if (years > 0) {
            result_.annual_return = (pow(1 + result_.total_return / 100, 1.0 / years) - 1) * 100;
        }
    }

    // 最大回撤
    result_.max_drawdown = calculateMaxDrawdown(result_.equity_curve);

    // 夏普比率
    if (result_.equity_curve.size() > 1) {
        std::vector<double> daily_returns;
        for (size_t i = 1; i < result_.equity_curve.size(); i++) {
            double daily_return = (result_.equity_curve[i] - result_.equity_curve[i - 1]) / result_.equity_curve[i - 1];
            daily_returns.push_back(daily_return);
        }
        result_.sharpe_ratio = calculateSharpeRatio(daily_returns);
    }

    // 胜率（简化计算）
    if (result_.total_trades > 0) {
        // 这里需要记录每笔交易的盈亏，简化处理
        result_.win_rate = 50.0; // 示例值
    }
}

double BacktestEngine::calculateSharpeRatio(const std::vector<double>& returns) {
    if (returns.empty()) return 0.0;

    double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();

    double variance = 0.0;
    for (double r : returns) {
        variance += (r - mean_return) * (r - mean_return);
    }
    variance /= returns.size();

    double std_dev = std::sqrt(variance);

    if (std_dev == 0) return 0.0;

    // 假设无风险利率为 0.03 (3%)
    double risk_free_rate = 0.03 / 250; // 日化
    return (mean_return - risk_free_rate) / std_dev * std::sqrt(250); // 年化
}

double BacktestEngine::calculateMaxDrawdown(const std::vector<double>& equity) {
    if (equity.empty()) return 0.0;

    double max_drawdown = 0.0;
    double peak = equity[0];

    for (double value : equity) {
        if (value > peak) {
            peak = value;
        }
        double drawdown = (peak - value) / peak * 100;
        if (drawdown > max_drawdown) {
            max_drawdown = drawdown;
        }
    }

    return max_drawdown;
}

double BacktestEngine::calculateWinRate(const std::vector<double>& trade_pnls) {
    if (trade_pnls.empty()) return 0.0;

    int wins = 0;
    for (double pnl : trade_pnls) {
        if (pnl > 0) wins++;
    }

    return (double)wins / trade_pnls.size() * 100;
}

BacktestResult BacktestEngine::getResult() const {
    return result_;
}

void BacktestEngine::printReport() const {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "        回测报告" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "初始资金: " << std::fixed << std::setprecision(2)
        << result_.initial_capital << " 元" << std::endl;
    std::cout << "最终资金: " << result_.final_capital << " 元" << std::endl;
    std::cout << "总盈亏: " << std::showpos << result_.total_pnl
        << std::noshowpos << " 元" << std::endl;
    std::cout << "总手续费: " << result_.total_fee << " 元" << std::endl;
    std::cout << "总收益率: " << std::setprecision(2) << result_.total_return << "%" << std::endl;
    std::cout << "年化收益率: " << result_.annual_return << "%" << std::endl;
    std::cout << "最大回撤: " << result_.max_drawdown << "%" << std::endl;
    std::cout << "夏普比率: " << result_.sharpe_ratio << std::endl;
    std::cout << "总交易次数: " << result_.total_trades << std::endl;
    std::cout << "胜率: " << result_.win_rate << "%" << std::endl;
    std::cout << "==========================================" << std::endl;
}