from backend.services.control_service import ControlService

control = ControlService()

# Chuyển sang chế độ AUTO
control.current_mode = "AUTO"

# Dữ liệu cảm biến giả lập
sensor_data = {
    "temperature": 24,
    "humidity": 50,
    "mq2": 20,
    "pm25": 15
}

# Kết quả AI giả lập
ai_result = {
    "air_quality": "good"
}

# Gọi Control Service
result = control.auto_control(ai_result, sensor_data)

print("Kết quả điều khiển:", result)