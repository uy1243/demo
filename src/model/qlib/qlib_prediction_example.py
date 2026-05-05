# qlib_prediction_example.py
import sys
import numpy as np
import pandas as pd
from qlib.data import D
from qlib.config import REG_CN, init
from qlib.workflow import R
from qlib.workflow.workflow import Workflow
from qlib.utils import init_instance_by_config
import qlib.tests.test_all_config as tc  # Importing config examples
from qlib.log import get_module_logger
import warnings
warnings.filterwarnings('ignore')

def init_qlib():
    """初始化 Qlib"""
    # 确保你已经按照之前的说明准备好了期货数据
    provider_uri = "~/.qlib/qlib_data/cn_data" 
    init(provider_uri=provider_uri, region=REG_CN)
    print("Qlib 初始化完成")

def define_dataset_and_model():
    """定义数据集、模型和实验配置"""
    
    # --- 1. 定义数据集配置 ---
    # 这个配置告诉 Qlib 如何获取特征和标签
    dataset_config = {
        "class": "DatasetH",
        "module_path": "qlib.data.dataset",
        "kwargs": {
            "handler": {
                "class": "Alpha158", # 使用 Alpha158 特征集，这是一个常用的特征工程模板
                "module_path": "qlib.contrib.data.handler", # 注意：这个 handler 需要 Qlib 安装了 contrib 模块
                "kwargs": {
                    "instruments": "csi300", # 注意：这里默认是 A 股指数，对于期货需要自定义或修改
                    "start_time": "2020-01-01",
                    "end_time": "2023-12-31",
                    "label": ["Ref($close, -20) / $close - 1"], # 预测未来20天的收益率作为标签
                },
            },
            "segments": {
                "train": ("2020-01-01", "2021-12-31"), # 训练集
                "valid": ("2022-01-01", "2022-06-30"), # 验证集
                "test": ("2022-07-01", "2023-12-31"), # 测试集
            },
        },
    }

    # 对于期货，我们可能需要自定义一个 Handler，因为 Alpha158 默认是为 A 股设计的
    # 这是一个简化的自定义 Handler 示例
    from qlib.data.dataset.handler import DataHandlerLP
    from qlib.data.dataset.processor import Processor
    from qlib.data.dataset import DatasetH

    class FuturesHandler(DataHandlerLP):
        def __init__(self, instruments="all", start_time=None, end_time=None, freq="day", fix_column=True):
            # 定义期货相关的特征
            # 这里的 fields 与 Alpha158 类似，但可以针对期货特性调整
            # 例如，加入持仓量相关的特征
            fields = [
                # 价格相关
                "Ref($close, 1)", "$high", "$low", "$close", "$open", "$volume", "$open_interest", # 基础价格/成交量/持仓量
                # 技术指标 (简化版)
                "Rank($close / Ref($close, 10))", # 10日价格排名
                "SMA($volume, 5) / $volume",      # 5日平均成交量相对于当日成交量
                "($high - $low) / $close",       # 波动率
                "Ref($close, -20) / $close - 1", # 标签: 未来20日收益率
            ]
            
            names = [
                "REF_CLOSE_1", "HIGH", "LOW", "CLOSE", "OPEN", "VOLUME", "OPEN_INTEREST",
                "RANK_PRICE_10D", "AVG_VOL_RATIO_5D", "HL_RATIO", "LABEL0" 
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

    # 重新定义 dataset_config 以使用自定义 Handler
    dataset_config = {
        "class": "DatasetH",
        "module_path": "qlib.data.dataset",
        "kwargs": {
            "handler": {
                "class": "FuturesHandler", # 这里需要注册类或使用全路径
                "module_path": __name__, # 当前模块
                "kwargs": {
                    "instruments": ["cu2509", "m2509"], # 替换为你实际拥有的期货合约
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

    # --- 2. 定义模型配置 ---
    # 选择一个模型，例如 LightGBM
    model_config = {
        "class": "LGBModel", # 或 "XGBModel", "LinearModel", "GRUModel" 等
        "module_path": "qlib.contrib.model.gbdt", # 对应的模块
        "kwargs": {
            "loss": "mse", # 损失函数，回归任务常用 mse
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

    # --- 3. 定义工作流配置 ---
    task_config = {
        "model": model_config,
        "dataset": dataset_config,
        "strategy": {
            "class": "TopkDropoutStrategy",
            "module_path": "qlib.contrib.strategy.signal_strategy",
            "kwargs": {
                "topk": 50, # 选择预测得分最高的50个合约
                "n_drop": 5, # 每次随机丢弃5个
            },
        },
        "backtest": {
            "start_time": "2022-07-01",
            "end_time": "2023-12-31",
            "account": 10000000, # 初始资金
            "benchmark": "SH000300", # 基准，期货可考虑用特定指数或忽略
            "exchange_kwargs": {
                "limit_threshold": 0.095, # 涨跌停限制
                "deal_price": "close", # 交易价格
                "open_cost": 0.00025, # 开仓手续费
                "close_cost": 0.00025, # 平仓手续费
                "min_cost": 5, # 最低手续费
            },
        },
    }

    return task_config

def run_prediction_workflow(task_config):
    """运行 Qlib 预测工作流"""
    logger = get_module_logger("Workflow")
    
    # --- 1. 初始化数据集和模型 ---
    dataset = init_instance_by_config(task_config["dataset"])
    model = init_instance_by_config(task_config["model"])

    # --- 2. 启动实验 ---
    with R.start(experiment_name="futures_prediction"):
        # --- 3. 训练模型 ---
        logger.info("开始训练模型...")
        model.fit(dataset)
        logger.info("模型训练完成。")

        # --- 4. 生成预测 ---
        # 在验证集上进行预测
        valid_dataset = dataset.prepare("valid")
        pred_score = model.predict(valid_dataset)
        
        # 获取对应的标签 (实际收益率) 用于评估
        label_data = valid_dataset.iloc[:, -1] # 假设最后一列是 LABEL0
        
        # 将预测分数和实际标签合并
        pred_result = pd.concat([pred_score, label_data], axis=1)
        pred_result.columns = ['score', 'label']
        pred_result.index.names = ['datetime', 'instrument']

        print("\n--- 预测结果示例 ---")
        print(pred_result.head(10))

        # --- 5. 评估预测效果 ---
        # 计算预测值与实际值的相关性 (IC - Information Coefficient)
        ic = pred_result.groupby(level=0).apply(lambda x: x['score'].corr(x['label'])).mean()
        print(f"\n验证集平均 IC (信息系数): {ic:.4f}")

        # 计算 rank IC (基于排序的相关性)
        rank_ic = pred_result.groupby(level=0).apply(lambda x: x['score'].rank().corr(x['label'].rank())).mean()
        print(f"验证集平均 Rank IC: {rank_ic:.4f}")

        # --- 6. 保存预测结果 ---
        output_file = "qlib_prediction_scores.csv"
        pred_result.to_csv(output_file)
        print(f"\n预测分数已保存到 {output_file}")

        # --- 7. 运行回测 (可选) ---
        # strategy = init_instance_by_config(task_config["strategy"])
        # backtest_config = task_config["backtest"]
        # report_normal, positions_normal = backtest(
        #     pred_score=pred_score, strategy=strategy, **backtest_config
        # )
        # summary = dict()
        # summary.update(report_normal.mean())
        # summary.update(positions_normal.mean())
        # print("\n--- 回测结果摘要 ---")
        # print(summary)

    print("\nQlib 预测工作流执行完毕。")


if __name__ == "__main__":
    # 1. 初始化
    init_qlib()

    # 2. 定义任务配置
    config = define_dataset_and_model()

    # 3. 运行预测
    run_prediction_workflow(config)