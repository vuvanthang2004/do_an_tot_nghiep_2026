
import json
import logging

import paho.mqtt.client as mqtt

#import các thông số cấu hình của mqtt ở trong config
from backend.config import (
    MQTT_BROKER,
    MQTT_PORT,
    TOPIC_SENSOR,
    TOPIC_COMMAND,
    TOPIC_DEVICE_STATUS,
)
#Dùng để show ra các hoạt động của các module đang chạy dù được hay không
logger = logging.getLogger(__name__)


class MQTTHandler:

    def __init__(self):
        self.client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id="air_quality_backend"
        )

        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

        self.on_sensor_data = None
        self.on_device_status = None

    def connect(self):
        self.client.connect(
            MQTT_BROKER,
            MQTT_PORT,
            keepalive=60
        )

        self.client.loop_start()

    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()

    def _on_connect(
        self,
        client,
        userdata,
        flags,
        reason_code,
        properties
    ):
        if reason_code.is_failure:
            logger.error("MQTT connection thất bại: %s", reason_code)
            return

        logger.info("Backend đã kết nối MQTT Broker")

        client.subscribe(TOPIC_SENSOR, qos=1)
        client.subscribe(TOPIC_DEVICE_STATUS, qos=1)

    def _on_message(self, client, userdata, msg):
        try:
            data = json.loads(msg.payload.decode("utf-8"))

            if not isinstance(data, dict):
                logger.warning("MQTT payload phải là JSON object")
                return

            if msg.topic == TOPIC_SENSOR:
                if self.on_sensor_data:
                    self.on_sensor_data(data)

            elif msg.topic == TOPIC_DEVICE_STATUS:
                if self.on_device_status:
                    self.on_device_status(data)

        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            logger.error("MQTT payload không hợp lệ: %s", exc)

        except Exception:
            logger.exception("Lỗi xử lý MQTT message")

    def publish_command(self, data: dict):
        payload = json.dumps(data)

        result = self.client.publish(
            TOPIC_COMMAND,
            payload,
            qos=1
        )

        if result.rc != mqtt.MQTT_ERR_SUCCESS:
            logger.error("Không thể gửi MQTT command")
            return False

        logger.info("Đã gửi MQTT command: %s", data)

        return True
