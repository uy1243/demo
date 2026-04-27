import grpc
from concurrent import futures
import time
import datetime
import threading
import pandas as pd
import mysql.connector
from transformers import pipeline
import requests
from bs4 import BeautifulSoup

# gRPC 自动生成
import news_analyze_pb2
import news_analyze_pb2_grpc

# ====================== 配置 ======================
MYSQL_CONFIG = {
    "host": "localhost",
    "user": "root",
    "password": "你的MySQL密码",
    "database": "quant_ctp",
    "charset": "utf8mb4"
}

# 金融情感模型（轻量、CPU可跑）
SENTIMENT_MODEL = "ProsusAI/finbert"
# 轻量摘要模型
SUMMARIZE_MODEL = "sshleifer/distilbart-cnn-12-6"

# 抓取关键词（期货相关）
CRAWL_KEYWORDS = ["棕榈油", "原油", "螺纹钢", "豆粕", "美联储", "减产", "进出口"]
GRPC_PORT = 50052
CRAWL_INTERVAL = 300  # 5分钟抓一次
# ==================================================

class NewsDB:
    def __init__(self):
        self.conn = mysql.connector.connect(**MYSQL_CONFIG)
        self.cursor = self.conn.cursor()

    def insert_news(self, title, content, publish_time, sentiment, summary, related, tags):
        try:
            sql = """
            INSERT IGNORE INTO news_analysis
            (title,content,publish_time,sentiment,summary,related_instruments,event_tags)
            VALUES (%s,%s,%s,%s,%s,%s,%s)
            """
            self.cursor.execute(sql, (title, content, publish_time, sentiment, summary, related, tags))
            self.conn.commit()
        except Exception as e:
            print("db err", e)

    def get_latest(self, minutes):
        cutoff = (datetime.datetime.now() - datetime.timedelta(minutes=minutes)).strftime("%Y-%m-%d %H:%M:%S")
        sql = "SELECT * FROM news_analysis WHERE publish_time >= %s ORDER BY publish_time DESC"
        self.cursor.execute(sql, (cutoff,))
        cols = [i[0] for i in self.cursor.description]
        return [dict(zip(cols, row)) for row in self.cursor.fetchall()]

    def get_by_instrument(self, instrument, minutes):
        cutoff = (datetime.datetime.now() - datetime.timedelta(minutes=minutes)).strftime("%Y-%m-%d %H:%M:%S")
        sql = """SELECT * FROM news_analysis
                 WHERE publish_time >= %s AND related_instruments LIKE %s
                 ORDER BY publish_time DESC"""
        self.cursor.execute(sql, (cutoff, f"%{instrument}%"))
        cols = [i[0] for i in self.cursor.description]
        return [dict(zip(cols, row)) for row in self.cursor.fetchall()]

# 模型加载
class FinAnalyzer:
    def __init__(self):
        self.sentiment_pipe = pipeline("text-classification", model=SENTIMENT_MODEL, device=-1)
        self.summarize_pipe = pipeline("summarization", model=SUMMARIZE_MODEL, device=-1)

    def sentiment(self, text):
        text = text[:512]
        res = self.sentiment_pipe(text)[0]
        score = res["score"]
        if res["label"] == "negative": return -score
        if res["label"] == "positive": return score
        return 0

    def summarize(self, text):
        if len(text) < 100: return text
        res = self.summarize_pipe(text[:2048], max_length=128, min_length=30, do_sample=False)
        return res[0]["summary_text"] if res else ""

# 东方财富新闻爬虫
class NewsCrawler:
    def __init__(self, analyzer, db):
        self.analyzer = analyzer
        self.db = db

    def crawl_eastmoney(self):
        headers = {"User-Agent": "Mozilla/5.0"}
        for kw in CRAWL_KEYWORDS:
            try:
                url = f"https://search.eastmoney.com/nh.html?q={kw}"
                r = requests.get(url, headers=headers, timeout=10)
                soup = BeautifulSoup(r.text, "html.parser")
                items = soup.select(".news-item")[:5]
                for item in items:
                    title = item.select_one("h3 a").get_text(strip=True)
                    href = item.select_one("h3 a")["href"]
                    cont = self.get_content(href)
                    pt = item.select_one(".time").get_text(strip=True)
                    if len(pt) < 16: continue
                    sentiment = self.analyzer.sentiment(cont)
                    summary = self.analyzer.summarize(cont)
                    related = self.match_instrument(title + cont)
                    tags = kw
                    self.db.insert_news(title, cont, pt, sentiment, summary, related, tags)
            except Exception as e:
                print("crawl err", kw, e)

    def get_content(self, url):
        try:
            r = requests.get(url, headers={"User-Agent": "Mozilla/5.0"}, timeout=8)
            soup = BeautifulSoup(r.text, "html.parser")
            ps = soup.select("p")
            return "\n".join(p.get_text(strip=True) for p in ps if len(p.get_text()) > 10)
        except:
            return ""

    def match_instrument(self, text):
        mapping = {
            "p2509": ["棕榈油", "马来", "印尼"],
            "rb2510": ["螺纹钢", "钢材", "铁矿"],
            "ma2509": ["甲醇"],
            "c2509": ["玉米"],
            "y2509": ["豆油", "油脂"]
        }
        res = []
        for inst, keys in mapping.items():
            for k in keys:
                if k in text:
                    res.append(inst)
                    break
        return ",".join(list(set(res)))

# gRPC 服务
class NewsServicer(news_analyze_pb2_grpc.NewsAnalyzeServiceServicer):
    def __init__(self, db):
        self.db = db

    def GetLatestNews(self, request, context):
        rows = self.db.get_latest(request.minutes)
        news = []
        for r in rows:
            news.append(news_analyze_pb2.NewsItem(
                title=r["title"],
                content=r["content"],
                publish_time=str(r["publish_time"]),
                sentiment=r["sentiment"],
                summary=r["summary"],
                related_instruments=r["related_instruments"] or ""
            ))
        return news_analyze_pb2.LatestNewsResp(success=True, msg="ok", news=news)

    def GetInstrumentSentiment(self, request, context):
        rows = self.db.get_by_instrument(request.instrument, request.minutes)
        total = 0.0
        news = []
        for r in rows:
            total += r["sentiment"]
            news.append(news_analyze_pb2.NewsItem(
                title=r["title"], summary=r["summary"], sentiment=r["sentiment"]
            ))
        avg = total / len(rows) if rows else 0
        return news_analyze_pb2.InstrumentResp(
            success=True, avg_sentiment=avg, news_count=len(rows), news=news
        )

# 定时抓取线程
def crawl_loop(crawler):
    while True:
        print(f"{datetime.datetime.now()} 开始抓取新闻")
        crawler.crawl_eastmoney()
        print("抓取完成，等待下一轮")
        time.sleep(CRAWL_INTERVAL)

# 主入口
def main():
    print("加载模型...")
    analyzer = FinAnalyzer()
    db = NewsDB()
    crawler = NewsCrawler(analyzer, db)

    # 启动定时抓取
    t = threading.Thread(target=crawl_loop, args=(crawler,), daemon=True)
    t.start()

    # 启动gRPC
    server = grpc.server(futures.ThreadPoolExecutor(10))
    news_analyze_pb2_grpc.add_NewsAnalyzeServiceServicer_to_server(NewsServicer(db), server)
    server.add_insecure_port(f"[::]:{GRPC_PORT}")
    server.start()
    print(f"新闻分析服务运行在 0.0.0.0:{GRPC_PORT}")
    server.wait_for_termination()

if __name__ == "__main__":
    main()