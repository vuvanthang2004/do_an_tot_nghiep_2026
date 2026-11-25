
import logging

from contextlib import asynccontextmanager
from fastapi import FastAPI, HTTPException, Request

from backend.services.mqtt_handler import MQTTHandler
from fastapi.middleware.cors import CORSMiddleware
from backend.services.sensor_service import SensorService
from backend.services.ai_service import analyze_air_quality
from backend.services.control_service import ControlService
from backend.services.device_service import DeviceService
from backend.database.database import get_connection
from backend.routers import auth

logging.basicConfig(level=logging.INFO)

logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan(app: FastAPI):

    # Khởi tạo các service
    app.state.sensor_service = SensorService()
    app.state.control_service = ControlService()
    app.state.device_service = DeviceService()

    app.state.ai_result = None

    # Khởi tạo MQTT
    mqtt_handler = MQTTHandler()
    app.state.mqtt_handler = mqtt_handler

    # Callback nhận dữ liệu cảm biến
    def handle_sensor_data(data):

        logger.info("Nhận dữ liệu cảm biến: %s", data)

        # 1. Kiểm tra và xử lý dữ liệu
        processed_data = app.state.sensor_service.process_sensor_data(data)

        if processed_data is None:
            return

        # 2. Gọi AI dự đoán
        ai_result = analyze_air_quality(processed_data)

        if ai_result is None:
            return

        app.state.ai_result = ai_result

        logger.info("Kết quả AI: %s", ai_result)

        # 3. Kiểm tra chế độ AUTO
        control_service = app.state.control_service

        if control_service.current_mode == "AUTO":

            # Chuẩn hóa nhãn AI
            normalized_result = dict(ai_result)

            if "air_quality" in normalized_result:
                normalized_result["air_quality"] = str(
                    normalized_result["air_quality"]
                ).lower()

            # 4. AI quyết định điều khiển thiết bị
            command = control_service.auto_control(
                normalized_result,
                processed_data
            )

            if command is not None:

                # 5. Lưu lệnh
                app.state.device_service.update_command(command)

                # 6. Gửi lệnh xuống ESP32 qua MQTT
                mqtt_handler.publish_command(command)

    # Callback nhận trạng thái thiết bị
    def handle_device_status(data):

        logger.info("Nhận trạng thái thiết bị: %s", data)

        app.state.device_service.process_device_status(data)

    mqtt_handler.on_sensor_data = handle_sensor_data
    mqtt_handler.on_device_status = handle_device_status

    # Kết nối MQTT
    mqtt_handler.connect()

    logger.info("Backend đã khởi động")

    try:
        yield

    finally:
        mqtt_handler.disconnect()

        logger.info("Backend đã dừng")


app = FastAPI(
    title="Air Quality Monitoring Backend",
    description="Backend giám sát chất lượng không khí sử dụng ESP32 và AI",
    version="1.0.0",
    lifespan=lifespan,
)

# 2. Cấu hình CORS cho Frontend
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(auth.router)
# =========================
# API ROOT
# =========================

@app.get("/")
def root():
    return {
        "message": "Air Quality Backend is running"
    }


# =========================
# API SENSOR
# =========================

@app.get("/api/sensors")
def get_sensors(request: Request):

    return request.app.state.sensor_service.get_latest_data()


# =========================
# API AI
# =========================

@app.get("/api/ai")
def get_ai_result(request: Request):

    return request.app.state.ai_result


# =========================
# API DEVICE
# =========================

@app.get("/api/devices")
def get_devices(request: Request):

    return request.app.state.device_service.get_latest_status()

@app.get("/test-db")
def test_database():
    try:
        conn = get_connection()

        cursor = conn.cursor()
        cursor.execute("SELECT VERSION()")

        version = cursor.fetchone()

        cursor.close()
        conn.close()

        return {
            "status": "success",
            "message": "Backend kết nối MySQL thành công!",
            "mysql_version": version[0]
        }

    except Exception as e:
        return {
            "status": "error",
            "message": str(e)
        }
# =========================
# API CONTROL MODE
# =========================

@app.post("/api/mode")
def set_mode(data: dict, request: Request):

    mode = data.get("mode")

    if not isinstance(mode, str):
        raise HTTPException(
            status_code=400,
            detail="Mode phải là AUTO hoặc MANUAL"
        )

    success = request.app.state.control_service.set_mode(mode)

    if not success:
        raise HTTPException(
            status_code=400,
            detail="Mode không hợp lệ"
        )

    return {
        "message": "Đã thay đổi chế độ",
        "mode": request.app.state.control_service.current_mode
    }


# =========================
# API MANUAL CONTROL
# =========================

@app.post("/api/control")
def manual_control(data: dict, request: Request):

    control_service = request.app.state.control_service

    # Chỉ cho phép điều khiển thủ công trong MANUAL
    if control_service.current_mode != "MANUAL":

        raise HTTPException(
            status_code=400,
            detail="Hãy chuyển sang chế độ MANUAL trước"
        )

    # Kiểm tra lệnh điều khiển
    command = control_service.manual_control(data)

    if command is None:

        raise HTTPException(
            status_code=400,
            detail="Lệnh điều khiển không hợp lệ"
        )

    # Lưu lệnh
    request.app.state.device_service.update_command(command)

    # Gửi MQTT xuống ESP32
    request.app.state.mqtt_handler.publish_command(command)

    return {
        "message": "Đã gửi lệnh điều khiển",
        "command": command
    }


