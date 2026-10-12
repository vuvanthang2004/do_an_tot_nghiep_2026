
import json
import random
import time

import paho.mqtt.client as mqtt


# ==================================================
# 1. CAU HINH MQTT BROKER
# ==================================================

BROKER_HOST = "127.0.0.1"
BROKER_PORT = 1883

SENSOR_TOPIC = "lab/air/sensor"
COMMAND_TOPIC = "lab/air/command"
DEVICE_STATUS_TOPIC = "lab/air/device_status"

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

def on_connect(client, userdata, flags, reason_code, properties):
    global running

    if reason_code == 0:
        running = True
        print("\n[MQTT] Ket noi Broker thanh cong!")

        # Dang ky nhan lenh tu Backend
        client.subscribe(COMMAND_TOPIC, qos=1)

        print("[MQTT] Da dang ky topic:", COMMAND_TOPIC)
    else:
        running = False
        print("[MQTT] Ket noi that bai:", reason_code)


# ==================================================
# 4. XU LY LENH NHAN DUOC QUA MQTT
# ==================================================

def on_message(client, userdata, message):
    global devices

    try:
        payload = message.payload.decode("utf-8")
        command = json.loads(payload)

        print("\n[MQTT] Nhan lenh:", command)

        changed = False

        for device in devices:
            if device in command:
                value = command[device]

                # Chi chap nhan trang thai 0 hoac 1
                if isinstance(value, bool) or value not in (0, 1):
                    print("[ERROR] Trang thai khong hop le:", device)
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
        print("[ERROR] Du lieu lenh MQTT khong hop le!")


# ==================================================
# 5. TAO DU LIEU CAM BIEN GIA LAP
# ==================================================

def read_simulated_sensors():
    """
    Tao du lieu gia lap thay cho cam bien that.

    Luu y:
    - mq2_raw la gia tri ADC gia lap.
    - dust_raw la gia tri ADC gia lap.
    - Chua phai ppm hoac PM2.5 da hieu chuan.
    """

    temperature = round(random.uniform(25.0, 35.0), 1)
    humidity = round(random.uniform(50.0, 80.0), 1)

    mq2_raw = random.randint(500, 2500)
    dust_raw = random.randint(500, 2000)

    data = {
        "temperature": temperature,
        "humidity": humidity,
        "mq2_raw": mq2_raw,
        "dust_raw": dust_raw,
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
                    print("\n[SENSOR] Gui du lieu:")
                    print(json.dumps(data, indent=2))

                    print("[DEVICE] Trang thai hien tai:")
                    print(json.dumps(devices, indent=2))
                else:
                    print("[MQTT] Gui du lieu that bai!")

            else:
                print("[MQTT] Dang cho ket noi Broker...")

            time.sleep(3)

    except KeyboardInterrupt:
        print("\n[INFO] Dang dung chuong trinh...")

    except OSError as error:
        print("[ERROR] Khong the ket noi MQTT Broker:", error)

    finally:
        client.loop_stop()
        client.disconnect()
        print("[INFO] Da ngat ket noi MQTT.")


if __name__ == "__main__":
    main()
