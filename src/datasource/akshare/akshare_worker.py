import sys
import json
import akshare as ak
import pandas as pd
from datetime import datetime, timedelta

def get_available_symbols():
    """获取可用的期货合约列表"""
    try:
        # 获取期货合约表
        symbols_df = ak.futures_symbol_table()
        return symbols_df['symbol'].tolist() if not symbols_df.empty else []
    except:
        try:
            # 尝试另一种方式获取合约列表
            main_contract_list = ak.futures_main_sina()
            if not main_contract_list.empty:
                return main_contract_list['symbol'].tolist()
        except:
            pass
        return []

def get_futures_quote(symbol):
    """获取期货实时报价"""
    try:
        # 标准化合约代码格式（移除可能的前缀）
        normalized_symbol = symbol.upper().strip()
        
        # 检查合约是否存在
        available_symbols = get_available_symbols()
        if available_symbols and normalized_symbol not in available_symbols:
            # 尝试匹配部分名称
            matched_symbols = [s for s in available_symbols if normalized_symbol in s or s in normalized_symbol]
            if matched_symbols:
                normalized_symbol = matched_symbols[0]  # 使用第一个匹配的符号
                print(f"Info: Using matched symbol {normalized_symbol} for {symbol}", file=sys.stderr)
        
        print(f"Debug: Looking for symbol {normalized_symbol}", file=sys.stderr)
        
        # 方案1: 尝试期货现货接口（最常用的实时数据接口）
        try:
            print("Trying futures_zh_spot...", file=sys.stderr)
            df = ak.futures_zh_spot()
            if not df.empty:
                # 尝试匹配合约
                filtered_df = df[df['symbol'].str.contains(normalized_symbol.replace('M', ''), case=False, na=False)]
                if filtered_df.empty:
                    # 尝试原始符号
                    filtered_df = df[df['symbol'].str.upper() == normalized_symbol]
                
                if not filtered_df.empty:
                    row = filtered_df.iloc[-1]
                    quote = {
                        "instrument": normalized_symbol,
                        "last": float(row.get("current", 0) or row.get("price", 0) or 0),
                        "open": float(row.get("open", 0) or 0),
                        "high": float(row.get("high", 0) or 0),
                        "low": float(row.get("low", 0) or 0),
                        "volume": int(row.get("volume", 0) or 0),
                        "open_interest": int(row.get("open_interest", 0) or 0),
                        "bid1": float(row.get("bid", 0) or 0),
                        "ask1": float(row.get("ask", 0) or 0),
                        "bid_vol1": 0,
                        "ask_vol1": 0,
                        "bid2": 0.0, "bid3": 0.0, "bid4": 0.0, "bid5": 0.0,
                        "ask2": 0.0, "ask3": 0.0, "ask4": 0.0, "ask5": 0.0,
                        "bid_vol2": 0, "bid_vol3": 0, "bid_vol4": 0, "bid_vol5": 0,
                        "ask_vol2": 0, "ask_vol3": 0, "ask_vol4": 0, "ask_vol5": 0,
                    }
                    
                    # 如果价格还是0，说明数据不可靠
                    if quote["last"] == 0:
                        print(f"Warning: Got zero prices for {normalized_symbol}, checking other sources", file=sys.stderr)
                    else:
                        print(f"Success: Got data for {normalized_symbol}", file=sys.stderr)
                        return quote
        except Exception as e:
            print(f"futures_zh_spot failed: {e}", file=sys.stderr)
        
        # 方案2: 尝试期货主力合约接口
        try:
            print("Trying futures_main_sina...", file=sys.stderr)
            df = ak.futures_main_sina(symbol=normalized_symbol)
            if not df.empty:
                row = df.iloc[-1]
                quote = {
                    "instrument": normalized_symbol,
                    "last": float(row.get("current_price", 0) or row.get("close", 0) or 0),
                    "open": float(row.get("open_price", 0) or row.get("open", 0) or 0),
                    "high": float(row.get("highest_price", 0) or row.get("high", 0) or 0),
                    "low": float(row.get("lowest_price", 0) or row.get("low", 0) or 0),
                    "volume": int(row.get("volume", 0) or 0),
                    "open_interest": int(row.get("open_interest", 0) or 0),
                    "bid1": 0.0,
                    "ask1": 0.0,
                    "bid_vol1": 0,
                    "ask_vol1": 0,
                    "bid2": 0.0, "bid3": 0.0, "bid4": 0.0, "bid5": 0.0,
                    "ask2": 0.0, "ask3": 0.0, "ask4": 0.0, "ask5": 0.0,
                    "bid_vol2": 0, "bid_vol3": 0, "bid_vol4": 0, "bid_vol5": 0,
                    "ask_vol2": 0, "ask_vol3": 0, "ask_vol4": 0, "ask_vol5": 0,
                }
                
                if quote["last"] != 0:
                    print(f"Success: Got data from futures_main_sina for {normalized_symbol}", file=sys.stderr)
                    return quote
        except Exception as e:
            print(f"futures_main_sina failed: {e}", file=sys.stderr)
        
        # 方案3: 尝试获取日线数据作为最新参考
        try:
            print("Trying futures_zh_daily_sina as fallback...", file=sys.stderr)
            df = ak.futures_zh_daily_sina(symbol=normalized_symbol)
            if not df.empty:
                row = df.iloc[-1]
                quote = {
                    "instrument": normalized_symbol,
                    "last": float(row.get("close", 0)),
                    "open": float(row.get("open", 0)),
                    "high": float(row.get("high", 0)),
                    "low": float(row.get("low", 0)),
                    "volume": int(row.get("volume", 0)),
                    "open_interest": 0,
                    "bid1": float(row.get("close", 0)) - 0.5 if float(row.get("close", 0)) > 0 else 0.0,
                    "ask1": float(row.get("close", 0)) + 0.5 if float(row.get("close", 0)) > 0 else 0.0,
                    "bid_vol1": 0,
                    "ask_vol1": 0,
                    "bid2": 0.0, "bid3": 0.0, "bid4": 0.0, "bid5": 0.0,
                    "ask2": 0.0, "ask3": 0.0, "ask4": 0.0, "ask5": 0.0,
                    "bid_vol2": 0, "bid_vol3": 0, "bid_vol4": 0, "bid_vol5": 0,
                    "ask_vol2": 0, "ask_vol3": 0, "ask_vol4": 0, "ask_vol5": 0,
                }
                
                if quote["last"] != 0:
                    print(f"Success: Got daily data as fallback for {normalized_symbol}", file=sys.stderr)
                    return quote
        except Exception as e:
            print(f"futures_zh_daily_sina fallback failed: {e}", file=sys.stderr)
        
        return {"error": f"无法获取 {symbol} 的有效数据，所有数据源都返回零值或失败"}
        
    except Exception as e:
        return {"error": f"获取数据时发生异常: {str(e)}"}

def get_historical_data(instrument, start_date, end_date):
    """获取历史数据"""
    try:
        # 标准化合约代码
        normalized_instrument = instrument.upper().strip()
        
        print(f"Getting historical data for {normalized_instrument} from {start_date} to {end_date}", file=sys.stderr)
        
        # 尝试期货日线数据
        try:
            df = ak.futures_zh_daily_sina(symbol=normalized_instrument)
            if not df.empty:
                df["date"] = pd.to_datetime(df["date"])
                df = df[(df["date"] >= pd.to_datetime(start_date)) & (df["date"] <= pd.to_datetime(end_date))]
                
                ticks = []
                for _, row in df.iterrows():
                    ticks.append({
                        "time": row["date"].strftime("%Y-%m-%d %H:%M:%S"),
                        "instrument": normalized_instrument,
                        "open": float(row.get("open", 0)),
                        "high": float(row.get("high", 0)),
                        "low": float(row.get("low", 0)),
                        "last": float(row.get("close", 0)),
                        "volume": int(row.get("volume", 0)),
                        "open_interest": int(row.get("open_interest", 0)) if pd.notna(row.get("open_interest", 0)) else 0
                    })
                
                if ticks:
                    print(f"Success: Got {len(ticks)} historical records", file=sys.stderr)
                    return {"ticks": ticks}
                else:
                    print("No historical data found in date range", file=sys.stderr)
        except Exception as e:
            print(f"Historical data query failed: {e}", file=sys.stderr)
        
        return {"error": f"无法获取 {instrument} 在 {start_date} 到 {end_date} 的历史数据"}
        
    except Exception as e:
        return {"error": f"获取历史数据时发生异常: {str(e)}"}

# ========================= 主入口 =========================
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(json.dumps({"error": "请输入: quote | hist"}))
        sys.exit(1)

    action = sys.argv[1]

    if action == "quote":
        symbol = sys.argv[2]
        print(f"Requesting quote for: {symbol}", file=sys.stderr)  # Debug info
        res = get_futures_quote(symbol)
        print(json.dumps(res, ensure_ascii=False))

    elif action == "hist":
        instrument = sys.argv[2]
        start = sys.argv[3]
        end = sys.argv[4]
        res = get_historical_data(instrument, start, end)
        print(json.dumps(res, ensure_ascii=False))