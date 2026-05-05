from qlib.data import D
from qlib.config import REG_CN, init

# 初始化 Qlib
provider_uri = "~/.qlib/qlib_data/cn_data" 
init(provider_uri=provider_uri, region=REG_CN)

# 尝试获取数据
instruments = ['cu2509', 'm2509']
fields = ['$close', '$volume', '$open_interest']
start_date = '2024-01-01'
end_date = '2024-12-31'

df = D.features(instruments, fields, start_date, end_date)
print(df.head())