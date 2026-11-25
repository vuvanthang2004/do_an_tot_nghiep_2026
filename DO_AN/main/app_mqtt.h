#ifndef APP_MQTT_H
#define APP_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* MQTT Broker */
#define MQTT_BROKER_URI "mqtt://192.168.0.107:1883"

/* MQTT Topics */
#define MQTT_TOPIC_SENSOR  "lab/air/sensor"
#define MQTT_TOPIC_COMMAND "lab/air/command"
#define MQTT_TOPIC_DEVICE  "lab/air/device_status"

/*
 * Kiểu callback nhận thông điệp MQTT
 */
typedef void (*app_mqtt_message_cb_t)(
    const char *topic,
    const char *payload
);

/* Khởi tạo MQTT */
esp_err_t app_mqtt_init(void);

/* Kiểm tra kết nối */
bool app_mqtt_is_connected(void);

/* Đăng ký callback nhận MQTT */
void app_mqtt_register_message_callback(
    app_mqtt_message_cb_t callback
);

/* Gửi dữ liệu cảm biến */
esp_err_t app_mqtt_publish_sensor_json(const char *json);

/* Gửi trạng thái thiết bị */
esp_err_t app_mqtt_publish_device_status(const char *json);

#ifdef __cplusplus
}
#endif

#endif