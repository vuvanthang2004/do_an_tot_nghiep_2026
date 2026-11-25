
#include "app_mqtt.h"

#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_event.h"

/* Thay bằng địa chỉ MQTT Broker của bạn */
#define MQTT_BROKER_URI       "mqtt://192.168.0.107:1883"

/* Các topic(đường dẫn logic)được sử dụng trong dự án */
#define MQTT_TOPIC_SENSOR     "lab/air/sensor"
#define MQTT_TOPIC_COMMAND    "lab/air/command"
#define MQTT_TOPIC_DEVICE     "lab/air/device_status"
/* static cho biết biến chỉ được dùng trong file.c này còn const cho biết được giá trị của biến này không thay đổi theo thời gian*/
/*dấu sao này cho thấy được TAG là một con trỏ và con trỏ này có lưu trữ địa chỉ của chuỗi "APP_MQTT" đang nằm trong ram */
static const char *TAG = "APP_MQTT";
/*esp_mqtt_client_handle_t là một kiểu dữ liệu con trỏ trỏ tới một struct, nó trỏ tới địa chỉ của một struct, trong struct đấy
lưu thông tin cần thiết để quản lý một MQTT client đang hoạt động*/
static esp_mqtt_client_handle_t mqtt_client = NULL;
/*kiểu cấu trúc là kiểu dữ liệu do người dùng định nghĩa cho phép nhiều biến với nhiều kiểu dữ liệu khác nhau lại thành một thể thống nhất*/
/*mqtt đã kết nối có nghĩa là nó có thể giao tiếp được với cả broker rồi đúng không, còn việc gửi hay nhận được hay không thì chưa biết*/
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
 * hàm xử lý các sự kiện MQTT trong chương trình ESP32, Nó chứa những công việc cần thực hiện khi MQTT xảy ra sự kiện
 */
static void mqtt_event_handler(
    void *handler_args,/*dữ liệu bổ sung mà bạn truyền khi đăng ký callback*/
    esp_event_base_t base,/*Xác định nhóm sự kiện*/
    int32_t event_id,/*Xác định sự kiện cụ thể đang xảy ra*/
    void *event_data/*Chứa thông tin chi tiết về sự kiện*/)

{
    /*chuyển kiểu con trỏ void nên chưa thể truy cập vào các trường của mqtt
    phải biến đổi chúng về kiểu dữ liệu của mqtt thì mới truy cập được
    các trường bên trong mqtt, event cũng là biến con trỏ trỏ tới struct*/
    esp_mqtt_event_handle_t event =
        (esp_mqtt_event_handle_t)event_data;
/*int32_t chỉ là mã số của các sự kiện thôi còn muốn hiểu rõ xem sự kiện cụ thể
đang xảy ra là gì thì phải đổi kiểu trả về*/
    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
        {
            mqtt_connected = true;

            ESP_LOGI(TAG, "đã kết nối với mqtt broker");

            /*
             * esp_mqtt_client_subscribe là hàm có kiểu trả về là int, nếu đăng ký thành công topic thì về 1, còn không được thì trả về -1
             */
            int msg_id = esp_mqtt_client_subscribe(
                event->client,/*event là một con trỏ trỏ tới địa chỉ của một cục dữ liệu lớn, trong dữ liệu lớn đấy có chứa
                rất nhiều trường, trong trường này có chứa client, client chứa địa chỉ tới mqtt client*/
                MQTT_TOPIC_COMMAND,/*chuyển hết các dữ liệu từ backend về topic này cho tôi*/
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

            ESP_LOGW(TAG, "mất kết nối với MQTT Broker");

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
                         "MQTT message bị chia thành nhiều phần");
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


/* Khởi tạo MQTT client */
esp_err_t app_mqtt_init(void)
{
    if (mqtt_client != NULL)
    {
        return ESP_OK;
    }
/*cấu hình mqtt client và chỉ cần địa chỉ của broker thôi*/
    esp_mqtt_client_config_t mqtt_config = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
/*sau khi câu hình xong thì khởi tạo mqtt_client chính là một con trỏ để trỏ tới đối tượng có kiểu dữ liệu là khối struct,
sau đó gán cho nó trỏ tới địa chỉ của khởi tạo client, hàm khởi tạo cần địa chỉ của cấu hình*/
    mqtt_client = esp_mqtt_client_init(&mqtt_config);
/*kiểm tra xem khởi tạo có được hay không*/
    if (mqtt_client == NULL)
    {
        ESP_LOGE(TAG, "không tạo được MQTT client");
        return ESP_FAIL;
    }
/*đăng ký hàm callback để tiếp nhận các thông tin về mqtt client trong quá trình hoạt động*/
    esp_err_t err = esp_mqtt_client_register_event(
        mqtt_client,/*sử dụng biến con trỏ này để cho biết được bạn muốn đăng ký callback cho mqtt client nào*/
        ESP_EVENT_ANY_ID,/*điều này đồng nghĩa với việc bạn muốn nhận tất cả các thông báo về mqtt_client*/
        mqtt_event_handler,/*khi mqtt xảy ra sự kiện sẽ gọi hàm này để xử lý sự kiện*/
        NULL
    );
/*esp_err_t là kiểu dữ liệu được ESP-IDF sử dụng để biểu diễn trạng thái thực hiện của một hàm*/
    if (err != ESP_OK)
    {
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return err;
    }
/*esp_mqtt_client_start() có nhiệm vụ khởi động MQTT client đã được khởi tạo trước đó,
 để ESP32 bắt đầu thực hiện các hoạt động kết nối và giao tiếp MQTT với Broker*/
    err = esp_mqtt_client_start(mqtt_client);

    if (err != ESP_OK)
    {
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
        return err;
    }

    ESP_LOGI(TAG, "đã khởi động MQTT client");

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
        mqtt_client,/*biến con trỏ giúp xác định được mqtt client cần phải gửi*/
        MQTT_TOPIC_SENSOR,/*đường dẫn logic mà bên gửi gửi vào broker*/
        json,/*chuỗi dữ liệu dạng json*/
        0,      /* Độ dài 0: payload là chuỗi kết thúc bằng '\0' */
        1,      /* QoS 1 */
        0       /* Không retain */
    );

    if (msg_id < 0)
    {
        ESP_LOGE(TAG, "gửi dữ liệu cảm biến thất bại");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "đã gửi dữ liệu cảm biến thành công, msg_id=%d",
             msg_id);

    return ESP_OK;
}

/* Gửi trạng thái các thiết bị lên broker */
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
        ESP_LOGE(TAG, "gửi trạng thái thiết bị thất bại");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG,
             "đã gửi trạng thái thiết bị, msg_id=%d",
             msg_id);

    return ESP_OK;
}
