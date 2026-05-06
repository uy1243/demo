import sys
import json
import akshare as ak
import pandas as pd
from datetime import datetime

def get_futures_quote(symbol):
    """获取期货实时行情"""
    try:
        futures_spot_price_df = ak.futures_spot_price(symbol=symbol)
        if futures_spot_price_df.empty:
            return {"error": f"No data found for {symbol}"}

        row = futures_spot_price_df.iloc

        # --- 尝试映射深度数据 ---
        # 注意：你需要打印 row.keys() 来确认 AKShare 实际返回的列名
        # 例如，它可能是 '买价', '卖价', '买量', '卖量'
        # AKShare 的实时接口可能不包含完整的5档深度，只有最优一档
        quote = {
            "instrument": row.get("合约", symbol),
            "last": float(row.get("最新价", 0)),
            "open": float(row.get("开盘价", 0)),
            "high": float(row.get("最高价", 0)),
            "low": float(row.get("最低价", 0)),
            "volume": int(row.get("成交量", 0)),
            "open_interest": float(row.get("持仓量", 0)),
            
            # 尝试映射最优一档深度
            # 注意：AKShare 列名通常是中文
            "bid1": float(row.get("买价", 0)), 
            "ask1": float(row.get("卖价", 0)),
            "bid_vol1": int(row.get("买量", 0)), # 这些列很可能不存在
            "ask_vol1": int(row.get("卖量", 0)), # 这些列很可能不存在
        }
        
        # --- 关键：如果深度数据列不存在，需要设置默认值 ---
        # 为了防止 C++ 代码访问不存在的 JSON 键导致崩溃，设置默认值
        if "bid1" not in quote:
            quote.update({
                "bid1": 0.0, "bid2": 0.0, "bid3": 0.0, "bid4": 0.0, "bid5": 0.0,
                "ask1": 0.0, "ask2": 0.0, "ask3": 0.0, "ask4": 0.0, "ask5": 0.0,
                "bid_vol1": 0, "bid_vol2": 0, "bid_vol3": 0, "bid_vol4": 0, "bid_vol5": 0,
                "ask_vol1": 0, "ask_vol2": 0, "ask_vol3": 0, "ask_vol4": 0, "ask_vol5": 0,
            })
        # 如果存在，则补充其他档位的默认值（AKShare 可能只提供一档）
        else:
            quote.update({
                "bid2": 0.0, "bid3": 0.0, "bid4": 0.0, "bid5": 0.0,
                "ask2": 0.0, "ask3": 0.0, "ask4": 0.0, "ask5": 0.0,
                "bid_vol2": 0, "bid_vol3": 0, "bid_vol4": 0, "bid_vol5": 0,
                "ask_vol2": 0, "ask_vol3": 0, "ask_vol4": 0, "ask_vol5": 0,
            })

        return quote

    except Exception as e:
        return {"error": str(e)}

def get_historical_data(instrument, start_date, end_date):
    """获取期货历史日线数据"""
    try:
        # AKShare 获取单个合约历史数据的接口
        # instrument: 例如 "M2609" (注意：这里可能需要大写或特定格式，需测试)
        # start_date, end_date: 格式 "YYYYMMDD"
        
        # 尝试获取数据
        # 注意：AKShare 的接口可能会因为交易所或合约代码格式略有不同
        df = ak.futures_zh_daily_sina(symbol=instrument, start_date=start_date, end_date=end_date)
        
        if df.empty:
            return {"error": f"No historical data found for {instrument} between {start_date} and {end_date}"}
        
        # 将 DataFrame 转换为 TickData 列表
        # 注意：历史数据的列名可能与实时数据不同，例如 "date", "open", "high", "low", "close", "volume"
        tick_list = []
        for index, row in df.iterrows():
            # 将每天的数据转换为 TickData 结构
            # 注意：这里假设历史数据的收盘价作为 'last' 价格
            tick = {
                # 时间戳可能需要根据实际数据调整，这里用日期字符串填充
                "time": row.get("date", ""), 
                "instrument": instrument,
                "open": float(row.get("open", 0)),
                "high": float(row.get("high", 0)),
                "low": float(row.get("low", 0)),
                "last": float(row.get("close", 0)), # 使用收盘价作为 last
                "volume": int(row.get("volume", 0)),
                # 历史日线数据通常没有持仓量字段，可能需要从其他接口获取或留空
                "open_interest": float(row.get("open_interest", 0)) if "open_interest" in row else 0 
            }
            tick_list.append(tick)

        return {"ticks": tick_list}

    except Exception as e:
        return {"error": str(e)}

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "Missing action argument (quote/hist)"}))
        sys.exit(1)

    action = sys.argv
    # 根据第一个参数决定执行哪个功能
    if action == "quote":
        if len(sys.argv) < 3:
            print(json.dumps({"error": "Missing symbol argument for quote"}))
            sys.exit(1)
        symbol = sys.argv
        result = get_futures_quote(symbol)
        print(json.dumps(result))
    elif action == "hist":
        if len(sys.argv) < 5:
            print(json.dumps({"error": "Missing instrument, start_time, end_time arguments for hist"}))
            sys.exit(1)
        instrument = sys.argv
        start_time = sys.argv
        end_time = sys.argv
        
        # 将 YYYY-MM-DD 格式转换为 YYYYMMDD
        start_formatted = start_time.replace("-", "")
        end_formatted = end_time.replace("-", "")
        
        result = get_historical_data(instrument, start_formatted, end_formatted)
        print(json.dumps(result))
    else:
        print(json.dumps({"error": "Invalid action. Use 'quote' or 'hist'"}))
        sys.exit(1)