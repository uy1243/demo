// events/event_system.h
#pragma once
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

// 定义事件基类
struct Event {
    std::string type;
    Event(const std::string& event_type) : type(event_type) {}
    virtual ~Event() = default;
};

// 市场数据更新事件
struct MarketUpdateEvent : public Event {
    MarketUpdateEvent() : Event("MARKET_UPDATE") {}
};

// 订单状态变更事件
struct OrderUpdateEvent : public Event {
    std::string order_id; // 本地订单ID
    std::string exchange_order_id; // 交易所订单ID (可选)
    OrderStatus new_status;
    int filled_volume;
    double avg_fill_price;
    std::string error_msg; // 错误信息

    OrderUpdateEvent(const std::string& id, OrderStatus status, int filled, double price, const std::string& msg = "")
        : Event("ORDER_UPDATE"), order_id(id), new_status(status), filled_volume(filled), avg_fill_price(price), error_msg(msg) {
    }
    // exchange_order_id 在 Account::on_order_update 中可以从本地订单记录里获取或通过映射获取
};

// 交易信号事件 (策略 -> 账户)
struct TradeSignalEvent : public Event {
    std::string instrument;
    Direction direction;
    double price;
    int volume;

    TradeSignalEvent(const std::string& inst, Direction dir, double px, int vol)
        : Event("TRADE_SIGNAL"), instrument(inst), direction(dir), price(px), volume(vol) {
    }
};

// 事件回调类型
template<typename T>
using EventCallback = std::function<void(const T&)>;

class EventSystem {
public:
    template<typename EventType>
    void subscribe(const std::string& event_type, EventCallback<EventType> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_[event_type].push_back([callback](const Event& e) {
            callback(static_cast<const EventType&>(e));
            });
    }

    void publish(const Event& event) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = callbacks_.find(event.type);
        if (it != callbacks_.end()) {
            for (auto& cb : it->second) {
                cb(event);
            }
        }
    }

private:
    std::unordered_map<std::string, std::vector<std::function<void(const Event&)>>> callbacks_;
    std::mutex mutex_;
};