import pandas as pd
import pymysql
import matplotlib.pyplot as plt

# ========== 1. 从MySQL读取数据 ==========
def load_data():
    conn = pymysql.connect(
        host="localhost",
        user="root",
        password="你的MySQL密码",
        database="quant",
        charset="utf8"
    )
    sql = """
        SELECT open, max, min, close, vol, cvol, tdate 
        FROM 你的表名 
        ORDER BY tdate
    """
    df = pd.read_sql(sql, conn)
    conn.close()
    
    df["tdate"] = pd.to_datetime(df["tdate"])
    df.set_index("tdate", inplace=True)
    return df

# ========== 2. 均线策略（简单双均线示例） ==========
def backtest_ma_strategy(df, fast=5, slow=20, initial_capital=100000):
    df = df.copy()
    df["ma_fast"] = df["close"].rolling(fast).mean()
    df["ma_slow"] = df["close"].rolling(slow).mean()

    # 信号生成
    df["signal"] = 0
    df.loc[df["ma_fast"] > df["ma_slow"], "signal"] = 1   # 多
    df.loc[df["ma_fast"] < df["ma_slow"], "signal"] = -1  # 空

    # 仓位（简化：每次信号变化时全仓）
    position = 0
    capital = initial_capital
    holdings = 0
    trade_log = []
    equity_curve = []

    for i, row in df.iterrows():
        close = row["close"]
        signal = row["signal"]

        if signal == 1 and position != 1:
            # 开多仓
            holdings = capital / close
            position = 1
            trade_log.append((row.name, "BUY", close, holdings))
        elif signal == -1 and position != -1:
            # 开空仓（这里简化为平仓）
            capital = holdings * close
            holdings = 0
            position = 0
            trade_log.append((row.name, "SELL", close, 0))

        # 记录权益
        equity = capital + holdings * close
        equity_curve.append((row.name, equity))

    equity_df = pd.DataFrame(equity_curve, columns=["time", "equity"])
    equity_df.set_index("time", inplace=True)

    # 收益统计
    final_capital = equity_curve[-1][1]
    total_return = (final_capital / initial_capital) - 1
    max_drawdown = ((equity_df["equity"].cummax() - equity_df["equity"]) / equity_df["equity"].cummax()).max()

    return {
        "equity": equity_df,
        "trades": trade_log,
        "final_capital": final_capital,
        "total_return": total_return,
        "max_drawdown": max_drawdown
    }

# ========== 3. 运行回测并绘图 ==========
if __name__ == "__main__":
    df = load_data()
    res = backtest_ma_strategy(df)

    print(f"初始资金: 100000")
    print(f"期末资金: {res['final_capital']:.2f}")
    print(f"总收益率: {res['total_return']:.2%}")
    print(f"最大回撤: {res['max_drawdown']:.2%}")

    # 绘制资金曲线
    plt.figure(figsize=(10, 5))
    plt.plot(res["equity"].index, res["equity"]["equity"], label="资金曲线")
    plt.title("回测资金曲线")
    plt.xlabel("时间")
    plt.ylabel("资金")
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.show()