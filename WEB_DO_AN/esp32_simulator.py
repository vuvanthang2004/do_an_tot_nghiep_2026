
import json
import random
import time

import paho.mqtt.client as mqtt


# ==================================================
# 1. CAU HINH MQTT BROKER
# ==================================================
# Địa chỉ của máy tính hiện tại cũng là địa chỉ của Broker
BROKER_HOST = "127.0.0.1"
BROKER_PORT = 1883
# đây như là một con đường làm ra để chuyển dữ liệu cảm biến từ esp lên backend
SENSOR_TOPIC = "lab/air/sensor"
#Đây là con đường truyền lệnh từ backend lên esp để điều khiển các thiết bị chuyển đổi nhiệt độ
COMMAND_TOPIC = "lab/air/command"
#ĐÂy là đường truyền để esp gửi trang thái của các thiết bị về backend xem nó bật hay tắt
DEVICE_STATUS_TOPIC = "lab/air/device_status"
# Đây là mã định danh để cho biết chính xác các client với nhau, để phân biệt rõ các client
CLIENT_ID = "esp32_simulator_01"

# ==================================================
# 2. TRANG THAI THIET BI GIA LAP
# ==================================================

devices = {
    "FAN": 0,
    "TEC": 0,
    "ION": 0
}

running = False


# ==================================================
# 3. XU LY KHI KET NOI MQTT THANH CONG
# ==================================================
# Hàm xử lý kết quả kết nối
def on_connect(client, userdata, flags, reason_code, properties):
    global running

    if reason_code == 0:
        running = True
        print("\n[MQTT] KẾT NỐI BROKER THÀNH CÔNG!")

        # Dang ky nhan lenh tu Backend
        client.subscribe(COMMAND_TOPIC, qos=1)

        print("[MQTT]ĐÃ ĐĂNG KÝ TOPIC:", COMMAND_TOPIC)
    else:
        running = False
        print("[MQTT]KẾT NỐI THẤT BẠI:", reason_code)


# ==================================================
# 4. XU LY LENH NHAN DUOC QUA MQTT
# ==================================================
# nhận lệnh MQTT từ Backend, cập nhật trạng thái thiết bị mô phỏng và gửi trạng thái mới về Backend
def on_message(client, userdata, message):
    global devices

    try:
        payload = message.payload.decode("utf-8")
        command = json.loads(payload)

        print("\n[MQTT] NHẬN LÊNH:", command)

        changed = False

        for device in devices:
            if device in command:
                value = command[device]

                # Chi chap nhan trang thai 0 hoac 1
                if isinstance(value, bool) or value not in (0, 1):
                    print("[ERROR] TRẠNG THÁI KHÔNG HỢP LỆ:", device)
                    continue

                devices[device] = value
                changed = True

                state = "ON" if value == 1 else "OFF"
                print(f"[DEVICE] {device}: {state}")

        # Gui lai trang thai thiet bi
        if changed:
            client.publish(
                DEVICE_STATUS_TOPIC,
                json.dumps(devices),
                qos=1
            )

    except (UnicodeDecodeError, json.JSONDecodeError, TypeError):
        print("[ERROR] DỮ LIỆU LỆNH MQTT KHÔNG HỢP LỆ!")


# ==================================================
# 5. TAO DU LIEU CAM BIEN GIA LAP
# ==================================================

def read_simulated_sensors():
    """
   tạo dữ liệu giả lập, đây là cách để kiểm tra xem dữ liệu được đưa vào backend hay không
    """

    temperature = round(random.uniform(40.0, 45.0), 1)
    humidity = round(random.uniform(50.0, 100.0), 1)

    mq2_raw = random.randint(500, 3000)
    dust_raw = random.randint(500, 3000)

    data = {
        "temperature": temperature,
        "humidity": humidity,
        "mq2": mq2_raw/ 4095 * 100,
        "pm25": dust_raw/ 4095 * 100,
        "timestamp": int(time.time())
    }

    return data


# ==================================================
# 6. CHUONG TRINH CHINH
# ==================================================

def main():
    client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id=CLIENT_ID
    )

    client.on_connect = on_connect
    client.on_message = on_message

    print("====================================")
    print("   ESP32 MQTT SIMULATOR")
    print("====================================")
    print("Broker:", BROKER_HOST, PORT if False else BROKER_PORT)
    print("Sensor topic:", SENSOR_TOPIC)
    print("Command topic:", COMMAND_TOPIC)

    try:
        # Ket noi den Mosquitto tren laptop
        client.connect(BROKER_HOST, BROKER_PORT, keepalive=60)

        # Xu ly ket noi va tin nhan MQTT o nen
        client.loop_start()

        while True:
            if running:
                data = read_simulated_sensors()
                payload = json.dumps(data)

                result = client.publish(
                    SENSOR_TOPIC,
                    payload,
                    qos=1
                )

                if result.rc == mqtt.MQTT_ERR_SUCCESS:
                    print("\n[SENSOR] GỬI DỮ LIỆU:")
                    print(json.dumps(data, indent=2))

                    print("[DEVICE] TRẠNG THÁI HIỆN TẠI:")
                    print(json.dumps(devices, indent=2))
                else:
                    print("[MQTT] GỬI DỮ LIỆU THẤT BẠI!")

            else:
                print("[MQTT] ĐANG KẾT NỐI CHO BROKER Broker...")

            time.sleep(3)

    except KeyboardInterrupt:
        print("\n[INFO] ĐANG DỪNG CHƯƠNG TRÌNH...")

    except OSError as error:
        print("[ERROR] KHÔNG THỂ KẾT NỐI MQTT Broker:", error)

    finally:
        client.loop_stop()
        client.disconnect()
        print("[INFO] ĐÃ NGẮT KẾT NỐI MQTT.")


if __name__ == "__main__":
    main()
