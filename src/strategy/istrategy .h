class IStrategy {
public:
    virtual void on_bar(const BarData& bar) = 0;
};