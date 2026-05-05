# download_futures_for_qlib_with_minute.py
import akshare as ak
import pandas as pd
import os
from datetime import datetime, timedelta
import time # For rate limiting if needed
import warnings
warnings.filterwarnings('ignore') # Suppress potential warnings from akshare/pandas

def download_multiple_futures_data_daily(symbols_config, start_date, end_date, output_dir="./qlib_futures_data"):
    """
    下载多个期货品种的日线数据，并按品种分别保存，便于后续转换为 Qlib 格式

    :param symbols_config: 字典，键为合约代码，值为品种名称 (用于组织文件)
                           例如: {'cu2509': 'copper', 'm2509': 'soybean_meal'}
    :param start_date: 开始日期，格式 'YYYY-MM-DD'
    :param end_date: 结束日期，格式 'YYYY-MM-DD'
    :param output_dir: 输出目录
    :return: combined_df, failed_list
    """
    daily_output_dir = os.path.join(output_dir, "daily")
    if not os.path.exists(daily_output_dir):
        os.makedirs(daily_output_dir)
        print(f"创建日线数据输出目录: {daily_output_dir}")

    all_data = []
    failed_symbols = []

    for symbol, variety in symbols_config.items():
        print(f"\n--- 正在下载 {symbol} ({variety}) 的日线数据... ---")
        try:
            # Step 1: Get all historical data for the specific contract
            print(f"  获取 {symbol} 的全部历史日线数据...")
            data_all_hist = ak.futures_zh_daily_sina(symbol=symbol.lower())
            
            if data_all_hist.empty:
                print(f"  警告: 未找到 {symbol} 的日线数据。")
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
                print(f"  警告: {symbol} 在 {start_date} 到 {end_date} 之间没有日线数据。")
                continue

            # Step 3: Add symbol and variety columns
            data_filtered['symbol'] = symbol.upper()
            data_filtered['variety'] = variety
            data_filtered['instrument'] = symbol.upper() # For Qlib compatibility

            # Step 4: Standardize column names for Qlib
            rename_map = {
                'date': 'datetime',
                'vol': 'volume',
                'amount': 'turnover',
                'hold': 'open_interest',
                'position': 'open_interest',
                'settle': 'settle_price',
            }
            for old_col, new_col in rename_map.items():
                if old_col in data_filtered.columns:
                    data_filtered.rename(columns={old_col: new_col}, inplace=True)

            # Ensure essential columns exist
            required_cols = ['datetime', 'open', 'high', 'low', 'close', 'volume', 'open_interest']
            for col in required_cols:
                if col not in data_filtered.columns:
                    if col == 'open_interest':
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
                             data_filtered[col] = 0
                    else:
                         print(f"    警告: 未找到 '{col}' 列，将填充默认值 0")
                         data_filtered[col] = 0
            
            # Sort by datetime
            data_filtered.sort_values(by='datetime', inplace=True)
            data_filtered.reset_index(drop=True, inplace=True)

            print(f"  成功下载 {symbol} 的 {len(data_filtered)} 条日线数据。")

            # Step 5: Save individual symbol data
            symbol_output_file = os.path.join(daily_output_dir, f"{symbol}_daily_{start_date.replace('-', '')}_to_{end_date.replace('-', '')}.csv")
            data_filtered.to_csv(symbol_output_file, index=False, encoding='utf-8')
            print(f"  {symbol} 日线数据已保存到 {symbol_output_file}")

            # Step 6: Append to overall dataset
            all_data.append(data_filtered)

            # Step 7: Rate limiting
            time.sleep(0.5)

        except Exception as e:
            print(f"  下载 {symbol} 日线数据时出错: {e}")
            failed_symbols.append(symbol)

    # Combine all daily data
    if all_data:
        combined_data = pd.concat(all_data, ignore_index=True)
        combined_data.sort_values(by=['instrument', 'datetime'], inplace=True)
        
        # Save combined daily data
        combined_output_file = os.path.join(daily_output_dir, f"all_futures_daily_{start_date.replace('-', '')}_to_{end_date.replace('-', '')}.csv")
        combined_data.to_csv(combined_output_file, index=False, encoding='utf-8')
        print(f"\n所有成功下载的日线数据已合并并保存到 {combined_output_file}")
        print(f"总共包含 {len(combined_data)} 条日线记录，涵盖 {combined_data['instrument'].nunique()} 个合约。")

        if failed_symbols:
            print(f"\n日线数据下载失败的合约: {failed_symbols}")
        
        return combined_data, failed_symbols
    else:
        print("\n没有成功下载任何日线数据。")
        if failed_symbols:
            print(f"失败的合约: {failed_symbols}")
        return pd.DataFrame(), failed_symbols


def download_multiple_futures_data_minute(symbols_config, periods=['1'], days_back=7, output_dir="./qlib_futures_data"):
    """
    下载多个期货品种的分钟线数据。
    注意：AkShare 的分钟线接口通常只提供近期数据，这里下载过去 N 天的数据作为示例。

    :param symbols_config: 字典，键为合约代码，值为品种名称
    :param periods: 分钟周期列表，例如 ['1', '5', '15']
    :param days_back: 下载过去多少天的数据
    :param output_dir: 输出目录
    :return: combined_df_minute, failed_minute_symbols
    """
    minute_output_dir = os.path.join(output_dir, "minute")
    if not os.path.exists(minute_output_dir):
        os.makedirs(minute_output_dir)
        print(f"创建分钟线数据输出目录: {minute_output_dir}")

    all_minute_data = []
    failed_minute_symbols = set() # Use set to avoid duplicates if same symbol fails for different periods

    # Calculate the date range for minute data (recent data only)
    end_date_minute = datetime.now().strftime('%Y-%m-%d')
    start_date_minute_obj = datetime.now() - timedelta(days=days_back)
    start_date_minute = start_date_minute_obj.strftime('%Y-%m-%d')
    print(f"分钟线数据下载时间范围: {start_date_minute} 到 {end_date_minute} (最近 {days_back} 天)")

    for symbol, variety in symbols_config.items():
        for period in periods:
            print(f"\n--- 正在下载 {symbol} ({variety}) 的 {period} 分钟线数据... ---")
            try:
                print(f"  获取 {symbol} 的 {period} 分钟线数据...")
                # Note: ak.futures_zh_minute_sina typically returns data for the *last few trading sessions*
                # It may not allow specifying a historical date range directly.
                # We assume it fetches the most recent available data.
                data_minute = ak.futures_zh_minute_sina(symbol=symbol.lower(), period=period)
                
                if data_minute.empty:
                    print(f"  警告: 未找到 {symbol} 的 {period} 分钟线数据。")
                    failed_minute_symbols.add(symbol)
                    continue

                # Check if data falls within our desired date range
                if 'datetime' in data_minute.columns:
                    data_minute['datetime'] = pd.to_datetime(data_minute['datetime'])
                    # Filter data within the calculated date range
                    data_minute_filtered = data_minute[
                        (data_minute['datetime'] >= pd.to_datetime(start_date_minute)) &
                        (data_minute['datetime'] <= pd.to_datetime(end_date_minute))
                    ]
                    
                    if data_minute_filtered.empty:
                        print(f"  警告: {symbol} 的 {period} 分钟线数据不在 {start_date_minute} 到 {end_date_minute} 范围内。")
                        continue # Skip saving if filtered data is empty
                    else:
                        data_minute = data_minute_filtered
                else:
                    print(f"  错误: {symbol} 的分钟线数据中未找到 'datetime' 列。")
                    failed_minute_symbols.add(symbol)
                    continue

                # Add symbol, variety, and period columns
                data_minute['symbol'] = symbol.upper()
                data_minute['variety'] = variety
                data_minute['instrument'] = symbol.upper()
                data_minute['period'] = period

                # Standardize column names for Qlib (similar to daily)
                rename_map = {
                    'vol': 'volume',
                    'amount': 'turnover',
                    'hold': 'open_interest',
                    'position': 'open_interest',
                    'settle': 'settle_price',
                }
                for old_col, new_col in rename_map.items():
                    if old_col in data_minute.columns:
                        data_minute.rename(columns={old_col: new_col}, inplace=True)

                # Ensure essential columns exist (similar to daily)
                required_cols = ['datetime', 'open', 'high', 'low', 'close', 'volume', 'open_interest']
                for col in required_cols:
                    if col not in data_minute.columns:
                        if col == 'open_interest':
                            possible_names = ['hold', 'position', 'oi']
                            found = False
                            for alt_name in possible_names:
                                if alt_name in data_minute.columns:
                                    data_minute.rename(columns={alt_name: 'open_interest'}, inplace=True)
                                    print(f"    重命名 '{alt_name}' 列为 'open_interest'")
                                    found = True
                                    break
                            if not found:
                                 print(f"    警告: 未找到 '{col}' 列，将填充默认值 0")
                                 data_minute[col] = 0
                        else:
                             print(f"    警告: 未找到 '{col}' 列，将填充默认值 0")
                             data_minute[col] = 0
            
                # Sort by datetime and instrument
                data_minute.sort_values(by=['instrument', 'datetime'], inplace=True)
                data_minute.reset_index(drop=True, inplace=True)

                print(f"  成功下载 {symbol} 的 {len(data_minute)} 条 {period} 分钟线数据。")

                # Save individual symbol-period data
                symbol_period_output_file = os.path.join(minute_output_dir, f"{symbol}_{period}min_{start_date_minute.replace('-', '')}_to_{end_date_minute.replace('-', '')}.csv")
                data_minute.to_csv(symbol_period_output_file, index=False, encoding='utf-8')
                print(f"  {symbol} ({period}min) 分钟线数据已保存到 {symbol_period_output_file}")

                # Append to overall dataset
                all_minute_data.append(data_minute)

                # Rate limiting for minute data too
                time.sleep(0.5)

            except Exception as e:
                print(f"  下载 {symbol} ({period}min) 分钟线数据时出错: {e}")
                failed_minute_symbols.add(symbol)

    # Combine all minute data
    if all_minute_data:
        combined_minute_data = pd.concat(all_minute_data, ignore_index=True)
        combined_minute_data.sort_values(by=['instrument', 'period', 'datetime'], inplace=True)
        
        # Save combined minute data
        combined_output_file_min = os.path.join(minute_output_dir, f"all_futures_minute_{start_date_minute.replace('-', '')}_to_{end_date_minute.replace('-', '')}.csv")
        combined_minute_data.to_csv(combined_output_file_min, index=False, encoding='utf-8')
        print(f"\n所有成功下载的分钟线数据已合并并保存到 {combined_output_file_min}")
        print(f"总共包含 {len(combined_minute_data)} 条分钟线记录，涵盖 {combined_minute_data['instrument'].nunique()} 个合约，周期 {combined_minute_data['period'].unique()}.")
        
        return combined_minute_data, list(failed_minute_symbols)
    else:
        print("\n没有成功下载任何分钟线数据。")
        return pd.DataFrame(), list(failed_minute_symbols)


def get_common_futures_symbols():
    """
    获取一些常见的期货品种合约代码，用于批量下载
    这次选择一些当前或近期可能活跃的合约代码
    """
    # 示例：选择一些在 2024-2025 年可能活跃的合约
    # 你需要根据实际市场情况更新这些代码
    # 例如，cu2509 在 2025 年 9 月到期，目前 (2024年5月) 可能还不是主力
    # 当前 (2024年5月) 的活跃合约可能是 cu2405, cu2406, cu2407, cu2408, cu2409, cu2410, cu2411, cu2412, cu2501, cu2502, cu2503, cu2504, cu2505, cu2506, cu2507, cu2508, cu2509, cu2510, cu2511, cu2512
    # 为了演示，我们选择一些可能在未来几个月内仍然活跃的合约
    # 请注意，你需要查询最新的合约列表来获取真正的活跃合约
    symbols_dict = {}

    # 选择一些可能在未来几个月内活跃的合约代码 (示例)
    # 铜 (SHFE) - 示例代码，需根据实际情况调整
    # 假设当前是 2024 年 5 月，我们选择 2024 年 6 月到 2025 年 3 月的合约
    copper_codes = [
        'cu2406', 'cu2407', 'cu2408', 'cu2409', 'cu2410', 'cu2411', 'cu2412',
        'cu2501', 'cu2502', 'cu2503', 'cu2504', 'cu2505', 'cu2506', 'cu2507', 'cu2508', 'cu2509'
    ]
    for code in copper_codes:
        symbols_dict[code] = 'copper'

    # 豆粕 (DCE)
    soy_meal_codes = [
        'm2405', 'm2407', 'm2408', 'm2409', 'm2411', 'm2412',
        'm2501', 'm2503', 'm2505', 'm2507', 'm2508', 'm2509'
    ]
    for code in soy_meal_codes:
        symbols_dict[code] = 'soybean_meal'

    # 螺纹钢 (SHFE)
    rebar_codes = [
        'rb2405', 'rb2410', 'rb2411', 'rb2412',
        'rb2501', 'rb2505', 'rb2510', 'rb2511'
    ]
    for code in rebar_codes:
        symbols_dict[code] = 'rebar'

    # 白糖 (ZCE)
    sugar_codes = [
        'sr2405', 'sr2407', 'sr2409', 'sr2411',
        'sr2501', 'sr2503', 'sr2505', 'sr2507', 'sr2509'
    ]
    for code in sugar_codes:
        symbols_dict[code] = 'sugar'

    # 玉米 (DCE)
    corn_codes = [
        'c2405', 'c2407', 'c2409', 'c2411',
        'c2501', 'c2503', 'c2505', 'c2507', 'c2509'
    ]
    for code in corn_codes:
        symbols_dict[code] = 'corn'

    # 热卷 (SHFE)
    hot_rolled_coil_codes = [
        'hc2405', 'hc2410', 'hc2411', 'hc2412',
        'hc2501', 'hc2505', 'hc2510', 'hc2511'
    ]
    for code in hot_rolled_coil_codes:
        symbols_dict[code] = 'hot_rolled_coil'

    # 沪深300股指期货 (CFFEX) - 示例
    if_codes = [
        'if2406', 'if2409', 'if2412',
        'if2503', 'if2506'
    ]
    for code in if_codes:
        symbols_dict[code] = 'index_future_IF'

    # 中证500股指期货 (CFFEX) - 示例
    ic_codes = [
        'ic2406', 'ic2409', 'ic2412',
        'ic2503', 'ic2506'
    ]
    for code in ic_codes:
        symbols_dict[code] = 'index_future_IC'

    # 国债期货 T (CFFEX) - 示例
    t_codes = [
        't2406', 't2409', 't2412',
        't2503', 't2506'
    ]
    for code in t_codes:
        symbols_dict[code] = 'bond_future_T'

    print(f"选择了 {len(symbols_dict)} 个可能活跃的合约代码用于下载。")
    return symbols_dict


if __name__ == "__main__":
    # --- 配置参数 ---
    # 1. 定义日线数据下载时间范围
    start_date_daily = "2020-01-01"
    end_date_daily = "2024-12-31"

    # 2. 定义分钟线数据下载参数 (通常只下载近期数据)
    minute_periods = ['1', '5'] # 下载 1 分钟和 5 分钟数据
    days_back_minute = 5 # 下载过去 5 个交易日的数据 (工作日)

    # 3. 定义要下载的合约
    symbols_to_download = get_common_futures_symbols()
    
    # Limiting for initial test
    max_symbols_to_test = 5 # Adjust as needed
    symbols_to_download = dict(list(symbols_to_download.items())[:max_symbols_to_test])
    print(f"限于测试，本次仅下载前 {max_symbols_to_test} 个合约: {list(symbols_to_download.keys())}")

    output_directory = "./qlib_futures_data_with_minute"

    # --- 执行日线数据下载 ---
    print("="*80)
    print(f"开始下载期货日线数据，时间范围: {start_date_daily} 到 {end_date_daily}")
    print(f"计划下载合约数量: {len(symbols_to_download)}")
    print(f"输出目录: {os.path.join(output_directory, 'daily')}")
    print("="*80)

    combined_df_daily, failed_list_daily = download_multiple_futures_data_daily(
        symbols_config=symbols_to_download,
        start_date=start_date_daily,
        end_date=end_date_daily,
        output_dir=output_directory
    )

    # --- 执行分钟线数据下载 ---
    print("\n" + "="*80)
    print(f"开始下载期货分钟线数据，周期 {minute_periods}，时间范围: 过去 {days_back_minute} 个交易日")
    print(f"计划下载合约数量: {len(symbols_to_download)}, 周期: {minute_periods}")
    print(f"输出目录: {os.path.join(output_directory, 'minute')}")
    print("="*80)

    combined_df_minute, failed_list_minute = download_multiple_futures_data_minute(
        symbols_config=symbols_to_download,
        periods=minute_periods,
        days_back=days_back_minute,
        output_dir=output_directory
    )

    # --- 最终汇总 ---
    print("\n" + "="*80)
    print("数据下载最终汇总:")
    
    if not combined_df_daily.empty:
        print(f"- 日线数据:")
        print(f"  - 总记录数: {len(combined_df_daily)}")
        print(f"  - 合约种类数: {combined_df_daily['instrument'].nunique()}")
        print(f"  - 日期范围: {combined_df_daily['datetime'].min()} 到 {combined_df_daily['datetime'].max()}")
        print(f"  - 已保存至: {os.path.join(output_directory, 'daily')}")
    else:
        print("- 日线数据: 未下载到任何数据")

    if not combined_df_minute.empty:
        print(f"- 分钟线数据:")
        print(f"  - 总记录数: {len(combined_df_minute)}")
        print(f"  - 合约种类数: {combined_df_minute['instrument'].nunique()}")
        print(f"  - 周期: {sorted(combined_df_minute['period'].unique())}")
        print(f"  - 日期范围: {combined_df_minute['datetime'].min()} 到 {combined_df_minute['datetime'].max()}")
        print(f"  - 已保存至: {os.path.join(output_directory, 'minute')}")
    else:
        print("- 分钟线数据: 未下载到任何数据 (可能是因为近期数据不足或合约不活跃)")

    if failed_list_daily or failed_list_minute:
        all_failed = set(failed_list_daily or []).union(set(failed_list_minute or []))
        print(f"\n下载失败的合约 (日线或分钟线): {list(all_failed)}")
    else:
        print("\n所有选定合约的日线和分钟线数据均下载成功！")

    print("="*80)
    print(f"所有数据已保存在 '{output_directory}' 目录下。")
    print("下一步可以将这些 CSV 数据转换为 Qlib 格式。")