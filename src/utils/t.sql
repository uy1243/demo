CREATE DATABASE IF NOT EXISTS quant_ctp;
USE quant_ctp;

-- 订单表
CREATE TABLE orders (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    order_ref VARCHAR(32) NOT NULL,
    instrument_id VARCHAR(16) NOT NULL,
    direction CHAR(1) NOT NULL, -- '0'买 '1'卖
    offset CHAR(1) NOT NULL,    -- '0'开 '1'平
    price DECIMAL(10,2) NOT NULL,
    volume INT NOT NULL,
    status CHAR(2) NOT NULL,    -- 订单状态
    front_id INT NOT NULL,
    session_id INT NOT NULL,
    order_sys_id VARCHAR(32) DEFAULT '',
    create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_order_ref(order_ref)
);

-- 成交表
CREATE TABLE trades (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    order_ref VARCHAR(32) NOT NULL,
    instrument_id VARCHAR(16) NOT NULL,
    trade_id VARCHAR(32) NOT NULL,
    direction CHAR(1) NOT NULL,
    offset CHAR(1) NOT NULL,
    price DECIMAL(10,2) NOT NULL,
    volume INT NOT NULL,
    trade_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_order_ref(order_ref)
);

-- 持仓表
CREATE TABLE positions (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    instrument_id VARCHAR(16) NOT NULL,
    direction CHAR(1) NOT NULL,
    total_volume INT DEFAULT 0,
    frozen_volume INT DEFAULT 0,
    open_price DECIMAL(10,2) DEFAULT 0,
    update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY uk_instr_dir(instrument_id, direction)
);