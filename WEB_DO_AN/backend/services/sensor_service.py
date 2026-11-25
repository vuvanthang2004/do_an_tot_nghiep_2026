# backend/services/sensor_service.py

import logging
from datetime import datetime

logger = logging.getLogger(__name__)

class SensorService:
    """
    Xử lý dữ liệu cảm biến nhận từ ESP32 thông qua MQTT.
    """
    def __init__(self):
        self.latest_data = None

    def process_sensor_data(self, data: dict):

# Kiểm tra dữ liệu cảm biến gửi đến từ handler có phải là dictionary hay không
        if not isinstance(data, dict):
            logger.error("Dữ liệu cảm biến không phải dictionary")
            return None

        # Các tệp dữ liệu cần có từ esp gửi đến
        required_fields = [
            "temperature",
            "humidity",
            "mq2",
            "pm25"
        ]

        # Kiểm tra xem các dữ liệu có đủ hay không
        for field in required_fields:
            if field not in data:
                logger.error("Thiếu trường dữ liệu: %s", field)
                return None
# chuyển đổi dữ liệu sang kiểu dữ liệu chuẩn 
        try:
            temperature = float(data["temperature"])
            humidity = float(data["humidity"])
            mq2 = float(data["mq2"])
            pm25 = float(data["pm25"])

        except (ValueError, TypeError):
            logger.error("Dữ liệu cảm biến không phải dạng số")
            return None

        # Kiểm tra nhiệt độ
        if not -40 <= temperature <= 120:
            logger.warning("Nhiệt độ không hợp lệ: %s", temperature)
            return None

        # Kiểm tra độ ẩm
        if not 0 <= humidity <= 150:
            logger.warning("Độ ẩm không hợp lệ: %s", humidity)
            return None

        # Kiểm tra MQ-2
        # Giả sử MQ-2 đã được chuyển thành phần trăm
        if not 0 <= mq2 <= 130:
            logger.warning("Giá trị MQ-2 không hợp lệ: %s", mq2)
            return None

        # Kiểm tra PM2.5
        if pm25 < 0:
            logger.warning("Giá trị PM2.5 không hợp lệ: %s", pm25)
            return None

        # Chuẩn hóa dữ liệu
        processed_data = {
            "temperature": temperature,
            "humidity": humidity,
            "mq2": mq2,
            "pm25": pm25,
            "timestamp": datetime.now().isoformat()
        }

        # Lưu dữ liệu cảm biến mới nhất trong bộ nhớ
        self.latest_data = processed_data

        logger.info("Đã xử lý dữ liệu cảm biến: %s", processed_data)

        return processed_data

    def get_latest_data(self):
        """
        Lấy dữ liệu cảm biến mới nhất.
        """
        return self.latest_data
