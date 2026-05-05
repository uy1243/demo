# qlib_data_service.py
import numpy as np
import pandas as pd
from qlib.data import D  # Qlib 数据API
from qlib.config import REG_CN, init
import argparse
import os
import json
from datetime import datetime, timedelta

def init_qlib():
    """初始化 Qlib"""
    # 初始化 Qlib，指定区域为中国市场 (REG_CN)
    # 你需要确保 ~/.qlib/qlib_data/cn_data 目录下有期货/期权数据
    # 如果没有，你需要使用 Qlib 的数据转换工具 (scripts/dump_bin.py) 将你的原始数据转为 Qlib 格式
    # 例如: python scripts/dump_bin.py --csv_path your_futures_csv_dir --qlib_dir ~/.qlib/qlib_data/cn_data --freq day --exclude_fields date,symbol
    
    # 这里假设你已经有了 Qlib 格式的期货数据
    provider_uri = "~/.qlib/qlib_data/cn_data" 
    init(provider_uri=provider_uri, region=REG_CN)
    print("Qlib 初始化完成")

def fetch_and_calculate_factors(instruments, start_date, end_date, factors_expr):
    """
    获取数据并计算因子

    Args:
        instruments (list): 期货/期权合约列表，例如 ['CU2509', 'm2509']
        start_date (str): 开始日期, 'YYYY-MM-DD'
        end_date (str): 结束日期, 'YYYY-MM-DD'
        factors_expr (dict): 因子表达式字典, e.g., {'alpha001': '(Ref($close, -1) / $close - 1)'}

    Returns:
        pd.DataFrame: 包含日期、合约、因子值的DataFrame
    """
    print(f"正在获取数据并计算因子...")
    # 构造 fields
    fields = ['$open', '$high', '$low', '$close', '$volume'] # 基础字段
    fields.extend(list(factors_expr.values())) # 添加因子表达式
    
    # 构造 instruments 字符串，Qlib 通常需要带前缀
    # 注意：这里的 symbol 格式需要与你转换进 Qlib 数据库的格式一致
    # 例如，如果是 cu2509, m2509 等
    qlib_instruments = instruments 

    # 使用 Qlib 的 D.features 获取数据和计算因子
    df = D.features(qlib_instruments, fields, start_date, end_date)
    
    # 重置索引，将 multi-index 展开
    df_reset = df.reset_index()
    df_reset.columns = ['instrument', 'datetime'] + fields[5:] # 假设前5个是基础字段，后面是因子
    
    # 重命名因子列
    factor_mapping = {v: k for k, v in factors_expr.items()} # 反向映射
    df_reset.rename(columns=factor_mapping, inplace=True)
    
    # 添加一个简单的信号列（示例：当 alpha001 > 0.01 时为 'BUY'）
    # 你可以在这里根据你的模型输出或因子值生成更复杂的信号
    df_reset['signal'] = df_reset.apply(lambda row: 'BUY' if row.get('alpha001', 0) > 0.01 else ('SELL' if row.get('alpha001', 0) < -0.01 else 'HOLD'), axis=1)
    
    print(f"数据获取和因子计算完成，共 {len(df_reset)} 条记录")
    return df_reset

def save_data_to_file(data_df, output_file):
    """将计算结果保存到文件"""
    print(f"正在保存数据到 {output_file}")
    # 保存为 CSV，方便 C++ 解析
    data_df.to_csv(output_file, index=False)
    print(f"数据已保存到 {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Qlib Data Service')
    parser.add_argument('--instruments', nargs='+', required=True, help='List of instruments (e.g., cu2509 m2509)')
    parser.add_argument('--start_date', type=str, required=True, help='Start date (YYYY-MM-DD)')
    parser.add_argument('--end_date', type=str, required=True, help='End date (YYYY-MM-DD)')
    parser.add_argument('--output_file', type=str, required=True, help='Output CSV file path')
    args = parser.parse_args()

    # 初始化
    init_qlib()

    # 定义要计算的因子 (示例)
    # 这里可以根据你的策略需求进行调整
    factors_to_calc = {
        'alpha001': '(Ref($close, -1) / $close - 1)', # 示例：简单收益率
        'volatility_5d': 'Std($close, 5) / Mean($close, 5)', # 示例：5日波动率
    }

    # 获取数据和计算因子
    result_df = fetch_and_calculate_factors(args.instruments, args.start_date, args.end_date, factors_to_calc)

    # 保存结果
    save_data_to_file(result_df, args.output_file)

    print("Qlib 数据服务执行完毕。")