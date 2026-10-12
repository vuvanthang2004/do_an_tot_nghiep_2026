
#include "app_control.h"

#include <string.h>
#include <stdbool.h>

#include "esp_log.h"
#include "cJSON.h"

#include "app_mqtt.h"
#include "app_device.h"

static const char *TAG = "APP_CONTROL";

#define MQTT_TOPIC_COMMAND "lab/air/command"

static app_control_mode_t current_mode = APP_MODE_AUTO;

static bool fan_state = false;
static bool tec_state = false;
static bool ion_state = false;

/* Kiểm tra và đọc giá trị thiết bị trong JSON */
static bool read_device_command(
    const cJSON *root,
    const char *key,
    bool *state
)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);

    /* Không có trường này: giữ nguyên trạng thái */
    if (item == NULL) {
        return true;
    }

    /* Chỉ chấp nhận số nguyên 0 hoặc 1 */
    if (!cJSON_IsNumber(item) ||
        (item->valuedouble != 0 && item->valuedouble != 1)) {
        ESP_LOGW(TAG, "Gia tri khong hop le cho %s", key);
        return false;
    }

    *state = (item->valueint == 1);
    return true;
}

/* Xử lý lệnh bật/tắt thiết bị */
static esp_err_t apply_device_commands(
    const cJSON *root
)
{
    bool new_fan = fan_state;
    bool new_tec = tec_state;
    bool new_ion = ion_state;

    if (!read_device_command(root, "FAN", &new_fan) ||
        !read_device_command(root, "TEC", &new_tec) ||
        !read_device_command(root, "ION", &new_ion)) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Chỉ gọi hàm điều khiển khi có lệnh hợp lệ.
     * Trạng thái phần mềm cập nhật sau khi GPIO được đặt thành công.
     */
    esp_err_t err;

    if (new_fan != fan_state) {
        err = fan_set_state(new_fan);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Khong dieu khien duoc FAN");
            return err;
        }
        fan_state = new_fan;
    }

    if (new_tec != tec_state) {
        err = tec_set_state(new_tec);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Khong dieu khien duoc TEC");
            return err;
        }
        tec_state = new_tec;
    }

    if (new_ion != ion_state) {
        err = ion_set_state(new_ion);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Khong dieu khien duoc ION");
            return err;
        }
        ion_state = new_ion;
    }

    ESP_LOGI(TAG,
             "Trang thai: FAN=%d, TEC=%d, ION=%d",
             fan_state, tec_state, ion_state);

    return ESP_OK;
}

/* Callback được app_mqtt.c gọi khi nhận được MQTT message */

static void app_control_mqtt_callback(
    const char *topic,
    const char *payload
)
{
    if (topic == NULL || payload == NULL ||
        strcmp(topic, MQTT_TOPIC_COMMAND) != 0) {
        return;
    }

    cJSON *root = cJSON_Parse(payload);

    if (root == NULL || !cJSON_IsObject(root)) {
        ESP_LOGW(TAG, "JSON khong hop le");
        cJSON_Delete(root);
        return;
    }

    const cJSON *source_item =
        cJSON_GetObjectItemCaseSensitive(root, "source");

    const cJSON *mode_item =
        cJSON_GetObjectItemCaseSensitive(root, "mode");

    /* Bắt buộc phải có nguồn lệnh WEB hoặc AI */
    if (!cJSON_IsString(source_item) ||
        source_item->valuestring == NULL) {
        ESP_LOGW(TAG, "Thieu source WEB/AI");
        cJSON_Delete(root);
        return;
    }

    bool from_web =
        strcmp(source_item->valuestring, "WEB") == 0;

    bool from_ai =
        strcmp(source_item->valuestring, "AI") == 0;

    if (!from_web && !from_ai) {
        ESP_LOGW(TAG, "Nguon lenh khong hop le");
        cJSON_Delete(root);
        return;
    }

    /*
     * Chỉ WEB được yêu cầu đổi chế độ.
     */
    if (mode_item != NULL) {
        if (!from_web ||
            !cJSON_IsString(mode_item) ||
            mode_item->valuestring == NULL) {
            ESP_LOGW(TAG, "Lenh thay doi mode khong hop le");
            cJSON_Delete(root);
            return;
        }

        if (strcmp(mode_item->valuestring, "AUTO") == 0) {
            current_mode = APP_MODE_AUTO;
        } else if (strcmp(mode_item->valuestring, "MANUAL") == 0) {
            current_mode = APP_MODE_MANUAL;
        } else {
            ESP_LOGW(TAG, "Mode khong hop le");
            cJSON_Delete(root);
            return;
        }

        ESP_LOGI(TAG, "Mode hien tai: %s",
                 current_mode == APP_MODE_AUTO
                     ? "AUTO" : "MANUAL");
    }

    /* Kiểm tra lệnh có yêu cầu điều khiển thiết bị không */
    bool has_device_command =
        cJSON_GetObjectItemCaseSensitive(root, "FAN") != NULL ||
        cJSON_GetObjectItemCaseSensitive(root, "TEC") != NULL ||
        cJSON_GetObjectItemCaseSensitive(root, "ION") != NULL;

    if (has_device_command) {

        /* WEB chỉ được điều khiển ở MANUAL */
        if (from_web && current_mode != APP_MODE_MANUAL) {
            ESP_LOGW(TAG,
                     "Tu choi lenh WEB: khong o MANUAL");
            cJSON_Delete(root);
            return;
        }

        /* AI chỉ được điều khiển ở AUTO */
        if (from_ai && current_mode != APP_MODE_AUTO) {
            ESP_LOGW(TAG,
                     "Tu choi lenh AI: khong o AUTO");
            cJSON_Delete(root);
            return;
        }

        esp_err_t err = apply_device_commands(root);

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Khong ap dung duoc lenh thiet bi");
        }
    }

    cJSON_Delete(root);
}


esp_err_t app_control_init(void)
{
    app_mqtt_register_message_callback(
        app_control_mqtt_callback
    );

    ESP_LOGI(TAG, "Da dang ky callback MQTT");
    return ESP_OK;
}

esp_err_t app_control_set_mode(app_control_mode_t mode)
{
    if (mode != APP_MODE_AUTO && mode != APP_MODE_MANUAL) {
        return ESP_ERR_INVALID_ARG;
    }

    current_mode = mode;

    ESP_LOGI(TAG, "Chuyen sang che do %s",
             mode == APP_MODE_AUTO ? "AUTO" : "MANUAL");

    return ESP_OK;
}

app_control_mode_t app_control_get_mode(void)
{
    return current_mode;
}

bool app_control_get_fan_state(void)
{
    return fan_state;
}

bool app_control_get_tec_state(void)
{
    return tec_state;
}

bool app_control_get_ion_state(void)
{
    return ion_state;
}
