# prediction_service.py
import numpy as np
import pandas as pd
from qlib.data import D
from qlib.config import REG_CN, init
from qlib.workflow import R
from qlib.utils import init_instance_by_config
import joblib # For saving/loading models
import argparse
import json
import time
import os
from datetime import datetime, timedelta

def init_qlib():
    """初始化 Qlib"""
    provider_uri = "~/.qlib/qlib_data/cn_data" 
    init(provider_uri=provider_uri, region=REG_CN)
    print("Qlib 初始化完成")

def load_or_train_model(model_path, dataset_config):
    """加载已训练模型，如果不存在则训练新模型"""
    if os.path.exists(model_path):
        print(f"加载已训练模型: {model_path}")
        # Note: Loading models depends on how you saved them.
        # If using joblib, it might work. For complex models trained by Qlib's workflow,
        # you might need to re-run the training part of the workflow or implement custom save/load.
        # This is a placeholder. You might need to adapt based on your specific model.
        try:
            model = joblib.load(model_path)
            return model
        except Exception as e:
            print(f"加载模型失败: {e}, 将重新训练")
    else:
        print(f"模型文件 {model_path} 不存在，将训练新模型")

    # --- Model Training Logic (Simplified) ---
    # In practice, you would run the full Qlib workflow to train.
    # Here, we'll just create a dummy model that generates random scores for demonstration.
    # You should replace this with the actual model loading/training logic from your workflow.
    
    # Example: Use Qlib's workflow to train, then save the model object.
    # This is complex and requires the full workflow setup from previous example.
    # For now, let's assume we have a pre-trained model object called 'trained_model'.
    # trained_model = ... # Run your workflow here...
    # joblib.dump(trained_model, model_path)
    
    # Dummy Model Placeholder
    class DummyModel:
        def predict(self, features_df):
            # Return a random score for each sample
            # In real case, this would be the actual model's predict method
            return pd.Series(np.random.randn(len(features_df)), index=features_df.index)

    print("使用占位模型 (Dummy Model)。请替换为你的实际模型训练/加载逻辑。")
    return DummyModel()

def generate_predictions(model, instruments, predict_start_date, predict_end_date):
    """生成预测"""
    print(f"正在为 {instruments} 从 {predict_start_date} 到 {predict_end_date} 生成预测...")

    # 1. 获取最新特征数据 (features)
    # 这里需要根据你的模型输入要求来定义 fields
    # 假设模型需要 ['CLOSE', 'VOLUME', 'OPEN_INTEREST', 'HL_RATIO', ...]
    fields = ['$close', '$volume', '$open_interest', '($high - $low) / $close'] 
    names = ['CLOSE', 'VOLUME', 'OPEN_INTEREST', 'HL_RATIO']
    
    # 注意：这里获取的是特征，而不是标签
    feature_data = D.features(instruments, fields, predict_start_date, predict_end_date, freq='day')

    if feature_data.empty:
        print(f"[WARNING] 在指定日期范围内没有找到 {instruments} 的特征数据。")
        return pd.DataFrame()

    # 2. 使用模型进行预测
    predictions = model.predict(feature_data)

    # 3. 将预测结果与原始数据（或索引）合并
    # predictions 是 Series, index 是 MultiIndex (datetime, instrument)
    # 我们将其转换为 DataFrame 便于处理
    pred_df = pd.DataFrame(predictions, columns=['score'])
    pred_df.index.names = ['datetime', 'instrument']
    
    # 4. (可选) 根据预测分数生成交易信号
    # 例如，score > 0.01 -> BUY, score < -0.01 -> SELL, else -> HOLD
    pred_df['signal'] = pred_df['score'].apply(lambda s: 'BUY' if s > 0.01 else ('SELL' if s < -0.01 else 'HOLD'))

    print(f"预测完成，共生成 {len(pred_df)} 条记录。")
    return pred_df

def save_predictions_to_json(predictions_df, output_file):
    """将预测结果保存为 JSON 文件"""
    if predictions_df.empty:
        print("预测结果为空，跳过保存。")
        return

    # 将 DataFrame 转换为嵌套字典格式，方便 C++ 解析
    # 格式: { "datetime": { "instrument": { "score": val, "signal": "BUY/SELL/HOLD" }, ... }, ... }
    result_dict = {}
    for (dt, inst), row in predictions_df.iterrows():
        if dt not in result_dict:
            result_dict[dt] = {}
        result_dict[dt][inst] = {
            "score": float(row['score']), # Ensure float type
            "signal": row['signal']
        }

    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(result_dict, f, indent=2, ensure_ascii=False)
    print(f"预测结果已保存到 {output_file}")

def run_prediction_cycle(model, instruments, output_file):
    """执行一次预测循环"""
    # 获取当前日期（或前一天，因为日线数据通常是收盘后）
    today = datetime.now().strftime('%Y-%m-%d')
    # 为了获取当天的预测，我们需要昨天的数据作为输入
    # 或者，如果模型可以处理实时数据，则可以使用今天的开盘数据
    # 这里假设我们使用昨天的数据预测今天的行为，即预测下一个bar
    yesterday = (datetime.now() - timedelta(days=1)).strftime('%Y-%m-%d')
    
    # 你可以根据需要调整预测的时间窗口
    predict_start_date = yesterday
    predict_end_date = today

    try:
        predictions = generate_predictions(model, instruments, predict_start_date, predict_end_date)
        save_predictions_to_json(predictions, output_file)
    except Exception as e:
        print(f"[ERROR] 预测过程中发生错误: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Qlib Prediction Service')
    parser.add_argument('--instruments', nargs='+', required=True, help='List of instruments (e.g., cu2509 m2509)')
    parser.add_argument('--model_path', type=str, required=True, help='Path to the trained model file (e.g., model.pkl)')
    parser.add_argument('--output_file', type=str, required=True, help='Output JSON file path for predictions')
    parser.add_argument('--interval', type=int, default=3600, help='Interval in seconds between prediction runs (default: 3600 = 1 hour)')
    args = parser.parse_args()

    init_qlib()

    # 加载模型
    model = load_or_train_model(args.model_path, None) # Pass appropriate dataset config if needed

    print(f"启动预测服务，每 {args.interval} 秒运行一次...")
    while True:
        run_prediction_cycle(model, args.instruments, args.output_file)
        print(f"等待 {args.interval} 秒后进行下一次预测...")
        time.sleep(args.interval)