
#ifndef APP_CONTROL_H
#define APP_CONTROL_H

#include "esp_err.h"
#include <stdbool.h>

typedef enum {
    APP_MODE_AUTO = 0,
    APP_MODE_MANUAL
} app_control_mode_t;

/* Đăng ký callback nhận lệnh MQTT */
esp_err_t app_control_init(void);

/* Đổi chế độ hoạt động */
esp_err_t app_control_set_mode(app_control_mode_t mode);

/* Đọc chế độ hiện tại */
app_control_mode_t app_control_get_mode(void);

/* Đọc trạng thái logic của thiết bị */
bool app_control_get_fan_state(void);
bool app_control_get_tec_state(void);
bool app_control_get_ion_state(void);

#endif /* APP_CONTROL_H */
