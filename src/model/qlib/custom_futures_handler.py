# custom_futures_handler.py
from qlib.data.dataset.handler import DataHandlerLP
import pandas as pd

class CustomFuturesHandler(DataHandlerLP):
    def __init__(self, instruments=["cu2509"], start_time=None, end_time=None, freq="day", fix_column=True):
        # 定义期货相关的特征和标签
        # 注意：这里的 fields 是 Qlib 表达式
        fields = [
            # --- Features (Input for the model) ---
            # Price-based features
            "$close", "$open", "$high", "$low",
            # Volume and Open Interest
            "$volume", "$open_interest",
            # Simple technical indicators (as expressions)
            "Ref($close, 1)", # Previous close
            "SMA($close, 5)", # 5-day SMA
            "($high - $low) / $close", # High-Low ratio / Volatility proxy
            "Rank($volume)", # Volume rank across instruments
            
            # --- Label (Target for the model) ---
            "Ref($close, -20) / $close - 1", # Future 20-day return (LABEL0)
        ]
        
        names = [
            "CLOSE", "OPEN", "HIGH", "LOW",
            "VOLUME", "OPEN_INTEREST",
            "PREV_CLOSE", "SMA5", "HL_RATIO", "VOLUME_RANK",
            "LABEL0" # The target variable
        ]

        super().__init__(
            instruments=instruments,
            start_time=start_time,
            end_time=end_time,
            freq=freq,
            fields=fields,
            names=names,
            dropna=True,
            fix_column=fix_column,
        )

# --- 如何在主训练脚本中使用 ---
# 在 qlib_train_example.py 的 define_task_config 函数中，修改 dataset_config:
"""
dataset_config = {
    "class": "DatasetH",
    "module_path": "qlib.data.dataset",
    "kwargs": {
        "handler": {
            "class": "CustomFuturesHandler", # Use your custom class
            "module_path": "custom_futures_handler", # Module where your class is defined
            "kwargs": {
                "instruments": ["cu2509", "m2509"], # Your futures contracts
                "start_time": "2020-01-01",
                "end_time": "2023-12-31",
            },
        },
        "segments": {
            "train": ("2020-01-01", "2021-12-31"),
            "valid": ("2022-01-01", "2022-06-30"),
            "test": ("2022-07-01", "2023-12-31"),
        },
    },
}
"""