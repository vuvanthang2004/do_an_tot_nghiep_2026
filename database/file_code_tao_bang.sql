
-- 1. Tạo database cho đồ án
CREATE DATABASE IF NOT EXISTS air_quality_db
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;

-- 2. Chọn database
USE air_quality_db;


-- 3. Bảng quản lý người dùng
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    fullname VARCHAR(100) NOT NULL,
    username VARCHAR(100) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);


-- 4. Bảng token đặt lại mật khẩu
CREATE TABLE IF NOT EXISTS password_reset_tokens (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    token_hash VARCHAR(255) UNIQUE NOT NULL,
    expires_at BIGINT NOT NULL,
    used TINYINT DEFAULT 0,

    FOREIGN KEY (user_id)
        REFERENCES users(id)
);


-- 5. Bảng lưu lịch sử dữ liệu cảm biến
CREATE TABLE IF NOT EXISTS sensor_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    temperature FLOAT,
    humidity FLOAT,
    mq2 FLOAT,
    pm25 FLOAT,
    `timestamp` DATETIME
);


-- 6. Bảng lưu trạng thái thiết bị
CREATE TABLE IF NOT EXISTS device_status (
    id INT AUTO_INCREMENT PRIMARY KEY,
    mode VARCHAR(20),
    fan TINYINT,
    tec TINYINT,
    ion TINYINT,
    updated_at DATETIME
);


-- 7. Kiểm tra các bảng đã tạo
SHOW TABLES;
