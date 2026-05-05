# qlib_train_example.py
import sys
import numpy as np
import pandas as pd
from qlib.data import D
from qlib.config import REG_CN, init
from qlib.workflow import R
from qlib.utils import init_instance_by_config
from qlib.model.trainer import TrainerR, train_model
import qlib.contrib.model.gbdt as gbdt_model # Example model module
import joblib
import os
import warnings
warnings.filterwarnings('ignore')

def init_qlib():
    """初始化 Qlib"""
    # 确保你已经按照之前的说明准备好了期货数据
    provider_uri = "~/.qlib/qlib_data/cn_data" 
    init(provider_uri=provider_uri, region=REG_CN)
    print("Qlib 初始化完成")

def define_task_config():
    """定义训练任务配置"""
    
    # --- 定义数据集配置 ---
    dataset_config = {
        "class": "DatasetH",
        "module_path": "qlib.data.dataset",
        "kwargs": {
            "handler": {
                # 使用 Qlib 内置的 Alpha158 特征集 (注意：它主要为 A 股设计，但可以作为基础)
                # 对于期货，你可能需要自定义 Handler
                "class": "Alpha158",
                "module_path": "qlib.contrib.data.handler", # Requires contrib module
                "kwargs": {
                    # 注意：'csi300' 是 A 股指数，对于期货，你需要替换为你的合约列表
                    # 或者使用 "all" 来包含所有数据，然后在训练时筛选
                    "instruments": "csi300", 
                    "start_time": "2020-01-01",
                    "end_time": "2023-12-31",
                    "label": ["Ref($close, -20) / $close - 1"], # 预测未来20天的收益率
                },
            },
            "segments": {
                "train": ("2020-01-01", "2021-12-31"), # 训练集
                "valid": ("2022-01-01", "2022-06-30"), # 验证集
                "test": ("2022-07-01", "2023-12-31"), # 测试集
            },
        },
    }

    # --- 定义模型配置 (LightGBM) ---
    model_config = {
        "class": "LGBModel", # Change to "XGBModel" for XGBoost, "LinearModel" for Linear, etc.
        "module_path": "qlib.contrib.model.gbdt", # Module containing the model class
        "kwargs": {
            "loss": "mse", # Loss function for regression
            "colsample_bytree": 0.8879,
            "learning_rate": 0.0421,
            "subsample": 0.8789,
            "lambda_l1": 205.6999,
            "lambda_l2": 580.9768,
            "max_depth": 8,
            "num_leaves": 210,
            "num_threads": 20,
        },
    }

    # --- 定义完整任务配置 ---
    task_config = {
        "model": model_config,
        "dataset": dataset_config,
        # Strategy and backtest configs are optional for pure training/prediction
        # but required if you want to run a full workflow including backtesting
        # "strategy": {...},
        # "backtest": {...},
    }
    
    return task_config

def train_and_save_model(task_config, model_save_path):
    """执行训练并保存模型"""
    
    print("开始模型训练流程...")
    
    # --- 1. 初始化数据集和模型 ---
    dataset = init_instance_by_config(task_config["dataset"])
    model = init_instance_by_config(task_config["model"])

    # --- 2. 启动 Qlib Experiment ---
    # 实验名称有助于区分不同的训练任务
    experiment_name = "my_futures_experiment"
    with R.start(experiment_name=experiment_name, recorder_name="recorder_1"):
        
        # --- 3. 训练模型 ---
        print("正在训练模型...")
        model.fit(dataset)
        print("模型训练完成。")

        # --- 4. (可选) 生成预测用于评估 ---
        print("正在生成验证集预测...")
        valid_dataset = dataset.prepare("valid")
        pred_score_valid = model.predict(valid_dataset)
        
        # 获取验证集的标签
        label_data_valid = valid_dataset.iloc[:, -1] # 假设最后一列是 LABEL0
        pred_result_valid = pd.concat([pred_score_valid, label_data_valid], axis=1)
        pred_result_valid.columns = ['score', 'label']
        
        # 计算 IC
        ic_valid = pred_result_valid.groupby(level=0).apply(lambda x: x['score'].corr(x['label'])).mean()
        print(f"验证集平均 IC: {ic_valid:.4f}")
        
        # --- 5. 保存模型 ---
        # Qlib 的模型对象通常可以直接序列化
        print(f"正在保存模型到 {model_save_path}...")
        joblib.dump(model, model_save_path)
        print(f"模型已保存到 {model_save_path}")

        # --- 6. (可选) 保存预测结果 ---
        # 你也可以将验证集或测试集的预测结果保存下来
        test_dataset = dataset.prepare("test")
        pred_score_test = model.predict(test_dataset)
        label_data_test = test_dataset.iloc[:, -1]
        pred_result_test = pd.concat([pred_score_test, label_data_test], axis=1)
        pred_result_test.columns = ['score', 'label']
        
        output_file = "training_predictions_test_set.csv"
        pred_result_test.to_csv(output_file)
        print(f"测试集预测结果已保存到 {output_file}")

    print("训练流程结束。")

def load_and_use_model(model_path):
    """加载已保存的模型并进行预测（示例）"""
    print(f"尝试加载模型 {model_path}...")
    try:
        loaded_model = joblib.load(model_path)
        print("模型加载成功！")
        # 在这里你可以用 loaded_model 做进一步的操作，比如预测新的数据
        return loaded_model
    except Exception as e:
        print(f"加载模型失败: {e}")
        return None

if __name__ == "__main__":
    # 1. 初始化
    init_qlib()

    # 2. 定义任务配置
    config = define_task_config()

    # 3. 定义模型保存路径
    model_save_path = "qlib_trained_model.pkl"

    # 4. 执行训练
    train_and_save_model(config, model_save_path)

    # 5. (可选) 验证加载
    loaded_model = load_and_use_model(model_save_path)