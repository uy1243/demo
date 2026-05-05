# download_futures_for_qlib.py
import akshare as ak
import pandas as pd
import os
from datetime import datetime, timedelta
import time # For rate limiting if needed

def download_multiple_futures_data(symbols_config, start_date, end_date, output_dir="./qlib_futures_data"):
    """
    下载多个期货品种的数据，并按品种分别保存，便于后续转换为 Qlib 格式

    :param symbols_config: 字典，键为合约代码，值为品种名称 (用于组织文件)
                           例如: {'cu2509': 'copper', 'm2509': 'soybean_meal'}
    :param start_date: 开始日期，格式 'YYYY-MM-DD'
    :param end_date: 结束日期，格式 'YYYY-MM-DD'
    :param output_dir: 输出目录
    """
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"创建输出目录: {output_dir}")

    all_data = []
    failed_symbols = []

    for symbol, variety in symbols_config.items():
        print(f"\n--- 正在下载 {symbol} ({variety}) 的数据... ---")
        try:
            # Step 1: Get all historical data for the specific contract
            print(f"  获取 {symbol} 的全部历史数据...")
            data_all_hist = ak.futures_zh_daily_sina(symbol=symbol.lower())
            
            if data_all_hist.empty:
                print(f"  警告: 未找到 {symbol} 的数据。")
                failed_symbols.append(symbol)
                continue

            # Step 2: Filter by date range
            print(f"  过滤日期范围 {start_date} 到 {end_date}...")
            data_all_hist['date'] = pd.to_datetime(data_all_hist['date'])
            data_filtered = data_all_hist[
                (data_all_hist['date'] >= pd.to_datetime(start_date)) &
                (data_all_hist['date'] <= pd.to_datetime(end_date))
            ].copy()

            if data_filtered.empty:
                print(f"  警告: {symbol} 在 {start_date} 到 {end_date} 之间没有数据。")
                # Even if filtered data is empty, original data might exist outside the range
                # Let's check if original data is also empty after date conversion
                if len(data_all_hist) == 0:
                     print(f"  信息: {symbol} 在整个历史范围内都没有数据。")
                else:
                     print(f"  信息: {symbol} 在整个历史范围内有 {len(data_all_hist)} 条数据，但都在指定范围外。")
                continue # Skip to next symbol

            # Step 3: Add symbol and variety columns
            data_filtered['symbol'] = symbol.upper()
            data_filtered['variety'] = variety
            data_filtered['instrument'] = symbol.upper() # For Qlib compatibility

            # Step 4: Standardize column names for Qlib
            # Common AkShare columns: date, open, high, low, close, volume, hold/position
            # Qlib expects: datetime, open, high, low, close, volume, open_interest
            rename_map = {
                'date': 'datetime',
                'vol': 'volume',
                'amount': 'turnover', # Turnover value if available
                'hold': 'open_interest', # Often called 'hold' in futures data
                'position': 'open_interest', # Alternative name for hold
                'settle': 'settle_price', # Settlement price if available
                # If settle is the only close-like price, map it carefully
                # 'settle': 'close', # Only do this if close is missing and settle is the primary closing price
            }
            # Apply renaming only if the old column exists
            for old_col, new_col in rename_map.items():
                if old_col in data_filtered.columns:
                    data_filtered.rename(columns={old_col: new_col}, inplace=True)

            # Ensure essential columns exist
            required_cols = ['datetime', 'open', 'high', 'low', 'close', 'volume', 'open_interest']
            for col in required_cols:
                if col not in data_filtered.columns:
                    if col == 'open_interest':
                        # It might be named differently or missing
                        possible_names = ['hold', 'position', 'oi']
                        found = False
                        for alt_name in possible_names:
                            if alt_name in data_filtered.columns:
                                data_filtered.rename(columns={alt_name: 'open_interest'}, inplace=True)
                                print(f"    重命名 '{alt_name}' 列为 'open_interest'")
                                found = True
                                break
                        if not found:
                             print(f"    警告: 未找到 '{col}' 列，将填充默认值 0")
                             data_filtered[col] = 0 # Default value if missing
                    else:
                         print(f"    警告: 未找到 '{col}' 列，将填充默认值 0")
                         data_filtered[col] = 0 # Default value if missing
            
            # Sort by datetime
            data_filtered.sort_values(by='datetime', inplace=True)
            data_filtered.reset_index(drop=True, inplace=True)

            print(f"  成功下载 {symbol} 的 {len(data_filtered)} 条日线数据。")

            # Step 5: Save individual symbol data (optional, good for inspection)
            symbol_output_file = os.path.join(output_dir, f"{symbol}_daily_{start_date.replace('-', '')}_to_{end_date.replace('-', '')}.csv")
            data_filtered.to_csv(symbol_output_file, index=False, encoding='utf-8')
            print(f"  {symbol} 数据已保存到 {symbol_output_file}")

            # Step 6: Append to overall dataset
            all_data.append(data_filtered)

            # Step 7: Rate limiting (be polite to the data source)
            time.sleep(0.5) # Pause briefly between requests

        except Exception as e:
            print(f"  下载 {symbol} 数据时出错: {e}")
            failed_symbols.append(symbol)

    # Combine all data into one large DataFrame (for potential bulk processing later)
    if all_data:
        combined_data = pd.concat(all_data, ignore_index=True)
        combined_data.sort_values(by=['instrument', 'datetime'], inplace=True)
        
        # Save combined data
        combined_output_file = os.path.join(output_dir, f"all_futures_daily_{start_date.replace('-', '')}_to_{end_date.replace('-', '')}.csv")
        combined_data.to_csv(combined_output_file, index=False, encoding='utf-8')
        print(f"\n所有成功下载的数据已合并并保存到 {combined_output_file}")
        print(f"总共包含 {len(combined_data)} 条记录，涵盖 {combined_data['instrument'].nunique()} 个合约。")

        if failed_symbols:
            print(f"\n下载失败的合约: {failed_symbols}")
        
        return combined_data, failed_symbols
    else:
        print("\n没有成功下载任何数据。")
        if failed_symbols:
            print(f"失败的合约: {failed_symbols}")
        return pd.DataFrame(), failed_symbols


def get_common_futures_symbols():
    """
    获取一些常见的期货品种合约代码，用于批量下载
    注意：你需要根据实际可用的合约和交割日期动态调整这些代码
    这里列出一些示例，你需要确保这些合约代码是有效的并且在指定日期范围内存在数据
    """
    # 示例合约代码列表 (需要根据实际情况调整)
    # 通常我们会选择当前或近期活跃的合约，或者历史上重要的合约
    # 以下是几个常见品种的假设性合约代码 (请根据实际情况替换)
    symbols_dict = {}
    
    # 铜 (SHFE)
    # 需要查询当前有效的CU合约，例如 CU2509, CU2510 等
    # 我们暂时使用一些示例代码，并在脚本中处理无效代码
    copper_codes = [f"cu{i:02d}{j:02d}" for i in range(20, 26) for j in [3, 6, 9, 12]] # e.g., cu2003, cu2006, ..., cu2512
    for code in copper_codes:
        symbols_dict[code] = 'copper'

    # 豆粕 (DCE)
    soy_meal_codes = [f"m{i:02d}{j:02d}" for i in range(20, 26) for j in [1, 3, 5, 7, 9, 11]] # e.g., m2001, m2003, ...
    for code in soy_meal_codes:
        symbols_dict[code] = 'soybean_meal'

    # 螺纹钢 (SHFE)
    rebar_codes = [f"rb{i:02d}{j:02d}" for i in range(20, 26) for j in [1, 5, 9]] # e.g., rb2001, rb2005, ...
    for code in rebar_codes:
        symbols_dict[code] = 'rebar'

    # 白糖 (ZCE)
    sugar_codes = [f"sr{i:02d}{j:02d}" for i in range(20, 26) for j in [1, 3, 5, 7, 9, 11]]
    for code in sugar_codes:
        symbols_dict[code] = 'sugar'

    # 玉米 (DCE)
    corn_codes = [f"c{i:02d}{j:02d}" for i in range(20, 26) for j in [1, 3, 5, 7, 9, 11]]
    for code in corn_codes:
        symbols_dict[code] = 'corn'

    # 热卷 (SHFE)
    hot_rolled_coil_codes = [f"hc{i:02d}{j:02d}" for i in range(20, 26) for j in [1, 5, 9]]
    for code in hot_rolled_coil_codes:
        symbols_dict[code] = 'hot_rolled_coil'

    # 沪深300股指期货 (CFFEX)
    # IF codes
    if_codes = [f"if{i:02d}{j:02d}" for i in range(20, 26) for j in [3, 6, 9, 12]]
    for code in if_codes:
        symbols_dict[code] = 'index_future_IF'

    # 中证500股指期货 (CFFEX)
    # IC codes
    ic_codes = [f"ic{i:02d}{j:02d}" for i in range(20, 26) for j in [3, 6, 9, 12]]
    for code in ic_codes:
        symbols_dict[code] = 'index_future_IC'

    # 国债期货 (T, TF, TS)
    # T codes
    t_codes = [f"t{i:02d}{j:02d}" for i in range(20, 26) for j in [3, 6, 9, 12]]
    for code in t_codes:
        symbols_dict[code] = 'bond_future_T'
    
    # TF codes
    tf_codes = [f"tf{i:02d}{j:02d}" for i in range(20, 26) for j in [3, 6, 9, 12]]
    for code in tf_codes:
        symbols_dict[code] = 'bond_future_TF'

    # 这些代码是生成的示例，你需要根据实际市场情况和 akshare 的数据覆盖范围进行调整
    # 例如，cu2003 在 2020 年 3 月已交割，现在可能无法获取数据
    # 你应该获取当前或近期有效的合约代码
    print(f"生成了 {len(symbols_dict)} 个合约代码用于下载。")
    return symbols_dict


if __name__ == "__main__":
    # --- 配置参数 ---
    # 1. 定义下载时间范围
    start_date = "2020-01-01" # 从 2020 年开始
    end_date = "2024-12-31"   # 到 2024 年底

    # 2. 定义要下载的合约 (可以使用预设的，也可以手动指定)
    # 使用预设的常见合约列表
    symbols_to_download = get_common_futures_symbols()
    
    # 或者手动指定一些特定的、当前活跃的合约 (示例)
    # 注意：你需要确保这些合约代码在 akshare 中存在且在指定时间范围内有数据
    # manual_symbols = {
    #     'cu2509': 'copper', 'cu2512': 'copper',
    #     'm2509': 'soybean_meal', 'm2511': 'soybean_meal',
    #     'rb2510': 'rebar', 'rb2601': 'rebar',
    #     'sr2509': 'sugar', 'c2509': 'corn',
    #     'hc2510': 'hot_rolled_coil',
    #     'if2506': 'index_future_IF', 'ic2506': 'index_future_IC',
    #     't2506': 'bond_future_T', 'tf2506': 'bond_future_TF',
    # }
    # symbols_to_download = manual_symbols

    # Limiting the number of symbols for initial testing to avoid too many requests
    # You can remove this limit when ready for full download
    max_symbols_to_test = 10 # Adjust this number as needed
    symbols_to_download = dict(list(symbols_to_download.items())[:max_symbols_to_test])
    print(f"限于测试，本次仅下载前 {max_symbols_to_test} 个合约: {list(symbols_to_download.keys())}")

    output_directory = "./qlib_futures_data_raw" # Specify output folder

    # --- 执行下载 ---
    print("="*60)
    print(f"开始下载期货数据，时间范围: {start_date} 到 {end_date}")
    print(f"计划下载合约数量: {len(symbols_to_download)}")
    print(f"输出目录: {output_directory}")
    print("="*60)

    combined_df, failed_list = download_multiple_futures_data(
        symbols_config=symbols_to_download,
        start_date=start_date,
        end_date=end_date,
        output_dir=output_directory
    )

    print("\n" + "="*60)
    if not combined_df.empty:
        print("数据下载汇总:")
        print(f"- 总记录数: {len(combined_df)}")
        print(f"- 合约种类数: {combined_df['instrument'].nunique()}")
        print(f"- 日期范围: {combined_df['datetime'].min()} 到 {combined_df['datetime'].max()}")
        print("- 各合约记录数:")
        print(combined_df['instrument'].value_counts())
        print(f"\n数据已保存至 '{output_directory}' 目录下的 CSV 文件中。")
        print("下一步可以将这些 CSV 数据转换为 Qlib 格式。")
    else:
        print("未能下载到任何数据。")

    if failed_list:
        print(f"\n以下合约下载失败，可能是因为合约代码不存在或在指定时间段内无数据: \n{failed_list}")

    print("="*60)