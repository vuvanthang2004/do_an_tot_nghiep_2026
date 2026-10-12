
#include "app_mqtt.h"

#include <string.h>

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_event.h"

/* Thay bằng địa chỉ MQTT Broker của bạn */
#define MQTT_BROKER_URI       "mqtt://192.168.0.107:1883"

/* Các topic sử dụng trong dự án */
#define MQTT_TOPIC_SENSOR     "lab/air/sensor"
#define MQTT_TOPIC_COMMAND    "lab/air/command"
#define MQTT_TOPIC_DEVICE     "lab/air/device_status"

static const char *TAG = "APP_MQTT";

static esp_mqtt_client_handle_t mqtt_client = NULL;

static bool mqtt_connected = false;

/* Callback do app_control.c hoặc module khác đăng ký */
static app_mqtt_message_cb_t message_callback = NULL;


/*
 * Đăng ký hàm nhận thông điệp từ Backend
 */
void app_mqtt_register_message_callback(
    app_mqtt_message_cb_t callback)
{
    message_callback = callback;
}


/*
 * Xử lý các sự kiện MQTT
 */
static void mqtt_event_handler(
    void *handler_args,
    esp_event_base_t base,
    int32_t event_id,
    void *event_data)
{
    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
        {
            mqtt_connected = true;

            ESP_LOGI(TAG, "Da ket noi MQTT Broker");

            /*
             * Đăng ký nhận lệnh từ Backend
             */
            int msg_id = esp_mqtt_client_subscribe(
                event->client,
                MQTT_TOPIC_COMMAND,
                1
            );

            ESP_LOGI(TAG,
                     "Subscribe topic %s, msg_id=%d",
                     MQTT_TOPIC_COMMAND,
                     msg_id);

            break;
        }

        case MQTT_EVENT_DISCONNECTED:
        {
            mqtt_connected = false;

            ESP_LOGW(TAG, "Mat ket noi MQTT Broker");

            /*
             * MQTT client sẽ tự thử kết nối lại
             * theo cơ chế của ESP-MQTT.
             */
            break;
        }

        case MQTT_EVENT_SUBSCRIBED:
        {
            ESP_LOGI(TAG,
                     "Subscribe thanh cong, msg_id=%d",
                     event->msg_id);
            break;
        }

        case MQTT_EVENT_PUBLISHED:
        {
            ESP_LOGI(TAG,
                     "Publish thanh cong, msg_id=%d",
                     event->msg_id);
            break;
        }

        case MQTT_EVENT_DATA:
        {
            /*
             * Ví dụ này chỉ xử lý message nằm trọn
             * trong một MQTT_EVENT_DATA.
             * Payload lệnh nên ngắn và không bị chia mảnh.
             */
            if (event->current_data_offset != 0 ||
                event->data_len != event->total_data_len)
            {
                ESP_LOGW(TAG,
                         "MQTT message bi chia thanh nhieu phan");
                break;
            }

            /*
             * Dữ liệu MQTT không đảm bảo kết thúc bằng '\0'.
             * Tạo bản sao có ký tự kết thúc chuỗi.
             */
            char topic[128];
            char payload[512];

            if (event->topic_len <= 0 ||
                event->topic_len >= sizeof(topic) ||
                event->data_len < 0 ||
                event->data_len >= sizeof(payload))
            {
                ESP_LOGW(TAG,
                         "Topic hoac payload qua dai");
                break;
            }

            memcpy(topic, event->topic, event->topic_len);
            topic[event->topic_len] = '\0';

            memcpy(payload, event->data, event->data_len);
            payload[event->data_len] = '\0';

            ESP_LOGI(TAG, "Nhan topic: %s", topic);
            ESP_LOGI(TAG, "Payload: %s", payload);

            /*
             * Chuyển dữ liệu cho module điều khiển xử lý.
             * Không bật relay trực tiếp tại đây.
             */
            if (message_callback != NULL)
            {
                message_callback(topic, payload);
            }

            break;
        }

        case MQTT_EVENT_ERROR:
        {
            ESP_LOGE(TAG, "MQTT co loi");

            if (event->error_handle != NULL)
            {
                ESP_LOGE(TAG,
                         "MQTT error type: %d",
                         event->error_handle->error_type);
            }

            break;
        }

        default:
        {
            break;
        }
    }
}


/*
 * Khởi tạo MQTT client
 */
esp_err_t app_mqtt_init(void)
{
    if (mqtt_client != NULL)
    {
        return ESP_OK;
    }

    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "Khong tao duoc MQTT client");
        return ESP_FAIL;
    }

    esp_err_t err = esp_mqtt_client_register_event(
        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL
    );

    if (err != ESP_OK)
    {
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return err;
    }

    err = esp_mqtt_client_start(mqtt_client);

    if (err != ESP_OK)
    {
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return err;
    }

    ESP_LOGI(TAG, "Da khoi dong MQTT client");

    return ESP_OK;
}


/*
 * Kiểm tra kết nối MQTT
 */
bool app_mqtt_is_connected(void)
{
    return mqtt_connected;
}


/*
 * Gửi dữ liệu cảm biến dưới dạng JSON
 */
esp_err_t app_mqtt_publish_sensor_json(const char *json)
{
    if (json == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (mqtt_client == NULL || !mqtt_connected)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_TOPIC_SENSOR,
        json,
        0,      /* Độ dài 0: payload là chuỗi kết thúc bằng '\0' */
        1,      /* QoS 1 */
        0       /* Không retain */
    );

    if (msg_id < 0)
    {
        ESP_LOGE(TAG, "Gui du lieu cam bien that bai");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "Da gui du lieu cam bien, msg_id=%d",
             msg_id);

    return ESP_OK;
}


/*
 * Gửi trạng thái các thiết bị
 */
esp_err_t app_mqtt_publish_device_status(const char *json)
{
    if (json == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (mqtt_client == NULL || !mqtt_connected)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_publish(
        mqtt_client,
        MQTT_TOPIC_DEVICE,
        json,
        0,
        1,
        0
    );

    if (msg_id < 0)
    {
        ESP_LOGE(TAG, "Gui trang thai thiet bi that bai");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "Da gui trang thai thiet bi, msg_id=%d",
             msg_id);

    return ESP_OK;
}
