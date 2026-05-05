# download_futures_minute.py
import akshare as ak
import pandas as pd
import os
from datetime import datetime, timedelta

def download_futures_minute(symbol, period='1', adjust='0'):
    """
    下载期货分钟线数据
    注意：此接口可能只返回最近几天的数据，不适合获取长期历史分钟线
    
    :param symbol: 期货合约代码，例如 'CU2509'
    :param period: 分钟周期，'1', '5', '15', '30', '60'
    :param adjust: 复权类型，'0' 不复权, '1' 后复权
    :return: 包含期货分钟线数据的 DataFrame
    """
    print(f"正在下载 {symbol} 的 {period} 分钟线数据...")
    try:
        # 注意：futures_zh_minute_sina 接口可能限制了历史数据的获取范围
        # 通常只能获取最近的分钟数据
        data = ak.futures_zh_minute_sina(symbol=symbol.upper(), period=period, adjust=adjust)
        
        if data.empty:
            print(f"警告: 未找到 {symbol} 的分钟线数据。")
            return pd.DataFrame()
        
        data['symbol'] = symbol.upper()
        data['period'] = period
        # Convert datetime column if it exists
        if 'datetime' in data.columns:
            data['datetime'] = pd.to_datetime(data['datetime'])
        elif 'date' in data.columns:
             data['datetime'] = pd.to_datetime(data['date'])
             data.drop(columns=['date'], inplace=True)
        
        print(f"成功下载 {symbol} 的 {len(data)} 条 {period} 分钟线数据。")
        return data

    except Exception as e:
        print(f"下载 {symbol} 分钟线数据时出错: {e}")
        return pd.DataFrame()

if __name__ == "__main__":
    # --- 配置参数 ---
    symbol = "cu2509" # Specific contract
    period = '1' # 1-minute bars

    # --- 下载数据 ---
    print("开始下载期货分钟线数据...")
    df_min = download_futures_minute(symbol, period)

    if not df_min.empty:
        print("分钟线数据预览:")
        print(df_min.head())
        print("数据形状:", df_min.shape)

        # --- 保存数据 ---
        start_time = df_min['datetime'].min().strftime('%Y%m%d_%H%M%S') if not df_min.empty else 'unknown'
        end_time = df_min['datetime'].max().strftime('%Y%m%d_%H%M%S') if not df_min.empty else 'unknown'
        output_filename = f"futures_minute_{symbol}_{period}min_{start_time}_to_{end_time}.csv"
        df_min.to_csv(output_filename, index=False, encoding='utf-8')
        print(f"分钟线数据已保存到 {output_filename}")
    else:
        print("未能下载到分钟线数据。")