from finnlp.data_sources.news.eastmoney import EastMoneyNews
from fingpt import FinGPT

# 1. 抓取期货新闻（示例：原油、PTA）
scraper=EastMoneyNews()
scraper.download(start_date="2026-04-01", end_date="2026-04-26", stock="原油")
news_df=scraper.dataframe  # 含：date, title, content, url

# 2. 加载FinGPT（7B中文金融版）
model=FinGPT.from_pretrained("AI4Finance/FinGPT-7B-Financial")

# 3. 逐条分析+情感打分
results = []
for _, row in news_df.iterrows():
    analysis=model.analyze(
        text=row["content"],
        task="sentiment+event+summary"  # 任务组合
    )
    results.append({
        "date": row["date"],
        "title": row["title"],
        "sentiment": analysis["sentiment"],  # -1(空)~+1(多)
        "event": analysis["events"],          # 关键事件列表
        "summary": analysis["summary"],      # 100字摘要
        "instruments": analysis["related_instruments"]  # 关联期货
    })

# 4. 写入MySQL（适配你的订单状态机库）
import pandas as pd
from sqlalchemy import create_engine
engine=create_engine("mysql://user:pwd@localhost:3306/futures_db")
pd.DataFrame(results).to_sql("news_analysis", engine, if_exists="append", index=False)