# fetch_contract_details.py
import akshare as ak

def fetch_contract_details(exchange=None):
    """
    获取期货合约详情
    :param exchange: 交易所代码，如 'SHFE', 'DCE', 'CZCE', 'CFFEX'
    """
    try:
        if exchange:
            details = ak.futures_contract_detail(exchange=exchange)
        else:
            # 获取所有交易所的合约详情 (如果接口支持)
            # This might not be available or might require multiple calls
            details_shfe = ak.futures_contract_detail(exchange="SHFE")
            details_dce = ak.futures_contract_detail(exchange="DCE")
            details_czce = ak.futures_contract_detail(exchange="CZCE")
            details_cffex = ak.futures_contract_detail(exchange="CFFEX")
            details = pd.concat([details_shfe, details_dce, details_czce, details_cffex], ignore_index=True)
        
        print(f"获取到 {len(details)} 条合约信息。")
        print("合约信息预览:")
        print(details.head(10))
        return details
    except Exception as e:
        print(f"获取合约详情时出错: {e}")
        return pd.DataFrame()

if __name__ == "__main__":
    # --- 获取上海期货交易所合约详情 ---
    shfe_details = fetch_contract_details(exchange="SHFE")
    if not shfe_details.empty:
        # Filter for actively traded symbols if needed
        # Example: Filter for Copper (cu) and Soybean Meal (m)
        active_symbols = shfe_details[shfe_details['variety'].str.lower().isin(['cu'])] # Add more varieties
        print("\n上海期货交易所铜相关合约:")
        print(active_symbols[['symbol', 'variety', 'product_id', 'delist_date']].head())
        
        dce_details = fetch_contract_details(exchange="DCE")
        if not dce_details.empty:
            dce_active = dce_details[dce_details['variety'].str.lower().isin(['m'])] # Soybean Meal
            print("\n大连商品交易所豆粕相关合约:")
            print(dce_active[['symbol', 'variety', 'product_id', 'delist_date']].head())