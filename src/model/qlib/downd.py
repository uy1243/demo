# download_futures_daily.py
import akshare as ak
import pandas as pd
import os
from datetime import datetime

def download_futures_daily(symbol_list, start_date, end_date, adjust='0'):
    """
    下载期货日线数据

    :param symbol_list: 期货合约代码列表，例如 ['CU2509', 'M2509'] 或单个代码 'CU2509'
    :param start_date: 开始日期，格式 'YYYYMMDD'
    :param end_date: 结束日期，格式 'YYYYMMDD'
    :param adjust: 复权类型，'0' 不复权, '1' 后复权
    :return: 包含期货数据的 DataFrame
    """
    all_data = []
    
    symbols = symbol_list if isinstance(symbol_list, list) else [symbol_list]

    for symbol in symbols:
        print(f"正在下载 {symbol} 的日线数据...")
        try:
            # 调用 AkShare 接口获取数据
            # 注意：合约代码通常使用主力连续合约代码或具体月份合约代码
            # 对于主力连续合约，有些接口可能有特殊标识
            # futures_daily_bar 接口可能需要具体月份合约代码，如 'CU2509'
            # 或者使用 futures_spot_stock 之类的接口获取主力合约
            # 这里先尝试 futures_daily_bar
            data = ak.futures_daily_sina(symbol=symbol, trade_date="")
            # 如果上面的接口不适用于历史区间，尝试如下接口
            # data = ak.futures_zh_daily_sina(symbol=symbol) # This gets all history, need filtering
            
            # 更通用的方法可能是 futures_zh_daily_sina 并手动过滤日期
            # 但 futures_zh_daily_sina 通常获取的是特定合约的全部历史
            # 我们使用 futures_daily_sina 逐日获取，但这效率较低
            # 更推荐的方法是获取主力合约或特定合约的历史
            
            # Let's use futures_zh_daily_sina which fetches history for a specific contract
            data = ak.futures_zh_daily_sina(symbol=symbol.upper()) # Ensure uppercase
            
            if data.empty:
                print(f"警告: 未找到 {symbol} 的数据。")
                continue
                
            # Filter by date range
            data['date'] = pd.to_datetime(data['date'])
            data_filtered = data[(data['date'] >= pd.to_datetime(start_date)) & 
                                 (data['date'] <= pd.to_datetime(end_date))]
            
            if data_filtered.empty:
                print(f"警告: {symbol} 在 {start_date} 到 {end_date} 之间没有数据。")
                continue

            data_filtered['symbol'] = symbol.upper() # Add symbol column
            all_data.append(data_filtered)
            print(f"成功下载 {symbol} 的 {len(data_filtered)} 条日线数据。")
            
        except Exception as e:
            print(f"下载 {symbol} 数据时出错: {e}")

    if all_data:
        combined_data = pd.concat(all_data, ignore_index=True)
        combined_data.sort_values(by=['symbol', 'date'], inplace=True)
        return combined_data
    else:
        print("没有成功下载任何数据。")
        return pd.DataFrame()

def save_data_to_csv(df, filename):
    """保存数据到 CSV 文件"""
    df.to_csv(filename, index=False, encoding='utf-8')
    print(f"数据已保存到 {filename}")

def get_main_contract_symbol_list():
    """获取主力合约代码列表 (示例)"""
    # AkShare 可能没有直接的主力合约列表接口
    # 通常需要自己构建，或者通过获取特定交易所的合约列表
    # 例如，获取上海期货交易所的合约
    # exchange_symbols = ak.futures_contract_detail(exchange="SHFE")
    # 然后根据规则判断主力合约 (通常是持仓量最大或成交量最大的近月合约)
    
    # For demo, let's define some common symbols manually
    # You can expand this list or fetch dynamically if needed
    # 注意：主力合约代码可能不是固定的，如 CU 主力可能从 CU2412 变为 CU2501
    # AkShare 提供了 futures_display_main_commodity 接口来获取主力连续数据
    main_contracts = ak.futures_display_main_sina() # 获取主力连续行情
    print("主力连续合约数据示例:")
    print(main_contracts.head())
    # This gives continuous prices, not historical for specific contracts
    # To get historical data for specific contracts, you need their exact codes
    
    # Example of getting specific contract codes
    # Get all SHFE contracts
    # shfe_contracts = ak.futures_contract_detail(exchange="SHFE")
    # print(shfe_contracts.head())
    # You would then filter for active contracts and select the ones you want
    
    # For this script, we'll proceed with user-defined specific contract codes
    return ["CU2509", "M2509", "RB2509"] # Example specific contract codes

if __name__ == "__main__":
    # --- 配置参数 ---
    # 你可以修改这些参数
    # 获取主力合约代码（但这主要用于获取连续数据，不一定适合历史K线）
    # main_symbols = get_main_contract_symbol_list()
    # 或者直接指定你想下载的具体合约代码
    specific_symbols = ["cu2509", "m2509", "rb2509"] # 注意：akshare 接口可能对大小写敏感，通常用大写
    specific_symbols = [s.upper() for s in specific_symbols] # Convert to upper
    start_date = "20200101" # YYYYMMDD
    end_date = "20241231"   # YYYYMMDD

    # --- 下载数据 ---
    print("开始下载期货日线数据...")
    df = download_futures_daily(specific_symbols, start_date, end_date)

    if not df.empty:
        # --- 数据预处理 (可选) ---
        # AkShare 返回的列名可能需要调整以符合 Qlib 的要求
        # Qlib 通常期望列名为: date, open, high, low, close, volume, open_interest 等
        # AkShare 的列名通常是: date, open, high, low, close, volume, ... 
        # Check AkShare's actual column names
        print("原始数据列名:", df.columns.tolist())
        print("原始数据预览:")
        print(df.head())

        # Rename columns if necessary to match expected format (example mapping)
        # This mapping might vary slightly depending on the exact akshare interface used
        rename_map = {
            'symbol': 'instrument',
            'date': 'datetime', # Qlib often uses datetime
            'open': 'open',
            'high': 'high',
            'low': 'low',
            'close': 'close',
            'volume': 'volume',
            'hold': 'open_interest', # Sometimes named 'hold' in akshare
            'position': 'open_interest', # Or sometimes 'position'
            'oi': 'open_interest', # Or sometimes 'oi'
            # Add other mappings as needed
        }
        # Only rename existing columns
        rename_map_filtered = {k: v for k, v in rename_map.items() if k in df.columns}
        df.rename(columns=rename_map_filtered, inplace=True)

        # Ensure datetime is in the correct format
        df['datetime'] = pd.to_datetime(df['datetime'])

        # Reorder columns if necessary (optional)
        # required_cols = ['datetime', 'instrument', 'open', 'high', 'low', 'close', 'volume', 'open_interest']
        # df = df[[col for col in required_cols if col in df.columns]]

        print("重命名后数据预览:")
        print(df.head())
        print("数据形状:", df.shape)

        # --- 保存数据 ---
        output_filename = f"futures_daily_{start_date}_to_{end_date}.csv"
        save_data_to_csv(df, output_filename)
    else:
        print("未能下载到任何数据，程序退出。")