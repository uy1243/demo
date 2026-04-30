import pandas as pd

def run_backtest(strategy, data_path):
    # 1. 读历史数据
    df = pd.read_csv(data_path)
    
    # 2. 初始化模拟交易
    trader = BacktestTrader()
    
    # 3. 逐K/Tick回放
    for idx, row in df.iterrows():
        tick = {
            'time': row['time'],
            'last': row['last'],
            'ask1': row['ask1'],
            'bid1': row['bid1'],
        }
        # 把tick喂给你的策略（和CTP实盘一样）
        strategy.on_tick(tick, trader)

    # 4. 回测报告
    print(f"最终资金: {trader.cash}")
    print(f"交易次数: {len(trader.trades)}")
    return trader