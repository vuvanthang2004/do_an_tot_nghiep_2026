
#ifndef APP_MQTT_H
#define APP_MQTT_H

#include <stdbool.h>
#include "esp_err.h"

/* Hàm được gọi khi nhận được MQTT message */
typedef void (*app_mqtt_message_cb_t)(
    const char *topic,
    const char *payload
);

/* Khởi tạo và bắt đầu MQTT client */
esp_err_t app_mqtt_init(void);

/* Kiểm tra MQTT đã kết nối hay chưa */
bool app_mqtt_is_connected(void);

/* Đăng ký hàm xử lý lệnh nhận được */
void app_mqtt_register_message_callback(
    app_mqtt_message_cb_t callback
);

/* Gửi JSON dữ liệu cảm biến */
esp_err_t app_mqtt_publish_sensor_json(
    const char *json
);

/* Gửi JSON trạng thái thiết bị */
esp_err_t app_mqtt_publish_device_status(
    const char *json
);

#endif
