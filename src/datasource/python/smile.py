import matplotlib.pyplot as plt

# 分离看涨、看跌
df_call = df_same_exp[df_same_exp["option_type"] == "C"].sort_values("strike_price")
df_put = df_same_exp[df_same_exp["option_type"] == "P"].sort_values("strike_price")

# 绘图
plt.figure(figsize=(10, 6))
plt.plot(df_call["strike_price"], df_call["IV"], "o-", label="看涨IV", color="red")
plt.plot(df_put["strike_price"], df_put["IV"], "s-", label="看跌IV", color="blue")
plt.axvline(x=F, color="gray", linestyle="--", label=f"标的价格={F}")
plt.xlabel("行权价（Strike）")
plt.ylabel("隐含波动率（IV）")
plt.title("豆粕期权波动率微笑曲线（同一到期日）")
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()