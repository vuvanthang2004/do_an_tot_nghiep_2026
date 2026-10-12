
import json
import logging

import paho.mqtt.client as mqtt

from backend.config import (
    MQTT_BROKER,
    MQTT_PORT,
    TOPIC_SENSOR,
    TOPIC_COMMAND,
    TOPIC_DEVICE_STATUS,
)

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

sensor_data = {}
device_status = {}


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code.is_failure:
        logger.error("MQTT connection failed: %s", reason_code)
        return

    logger.info("Connected to MQTT Broker")
    client.subscribe(TOPIC_SENSOR)
    client.subscribe(TOPIC_DEVICE_STATUS)


def on_message(client, userdata, message):
    global sensor_data, device_status

    try:
        payload = message.payload.decode("utf-8")
        data = json.loads(payload)

        if not isinstance(data, dict):
            raise ValueError("JSON must be an object")

        if message.topic == TOPIC_SENSOR:
            sensor_data = data
            logger.info("Sensor data: %s", sensor_data)

        elif message.topic == TOPIC_DEVICE_STATUS:
            device_status = data
            logger.info("Device status: %s", device_status)

    except (UnicodeDecodeError, json.JSONDecodeError, ValueError) as exc:
        logger.warning("Invalid MQTT message: %s", exc)


def publish_command(command: dict):
    payload = json.dumps(command)

    result = client.publish(
        TOPIC_SENSOR.replace("/sensor", "/command"),
        payload,
        qos=1,
    )

    if result.rc != mqtt.MQTT_ERR_SUCCESS:
        logger.error("Failed to publish command")


client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    client_id="air_quality_backend",
)

client.on_connect = on_connect
client.on_message = on_message


def start_mqtt():
    client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
    client.loop_start()


def stop_mqtt():
    client.loop_stop()
    client.disconnect()
