import tqsdk
import pandas as pd

# 初始化API
api = tqsdk.TqApi()
quote = api.get_quote("DCE.m2509")  # 标的期货：豆粕2509

# 获取该标的下所有期权合约
options = api.query_options(underlying="DCE.m2509")  # 关键：同一标的期权列表

# 转DataFrame
df_opt = pd.DataFrame(options)
# 筛选：同一到期日（只取近月）
exp_date = df_opt["expired"].min()  # 最近到期日
df_same_exp = df_opt[df_opt["expired"] == exp_date].copy()

# 订阅行情并获取最新价格
for idx, row in df_same_exp.iterrows():
    opt_quote = api.get_quote(row["instrument_id"])
    df_same_exp.at[idx, "bid_price1"] = opt_quote.bid_price1
    df_same_exp.at[idx, "ask_price1"] = opt_quote.ask_price1
    df_same_exp.at[idx, "last_price"] = opt_quote.last_price

# 只保留看涨(C)、看跌(P)，过滤异常
df_same_exp = df_same_exp[df_same_exp["option_type"].isin(["C", "P"])]
df_same_exp = df_same_exp.dropna(subset=["last_price", "strike_price"])

print("同一到期日期权列表：")
print(df_same_exp[["instrument_id", "strike_price", "option_type", "last_price"]])