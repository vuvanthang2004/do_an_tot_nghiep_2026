
#include "app_wifi.h"

#include <stdbool.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/event_groups.h"


#define WIFI_SSID       "TP-Link_4CC4"
#define WIFI_PASSWORD   "93945271"

/* Bit báo hiệu đã kết nối Wi-Fi và nhận IP */
#define WIFI_CONNECTED_BIT BIT0

static const char *TAG = "APP_WIFI";

static EventGroupHandle_t wifi_event_group;

static bool wifi_initialized = false;

static esp_event_handler_instance_t wifi_handler_instance;
static esp_event_handler_instance_t ip_handler_instance;


/*
 * Xử lý sự kiện Wi-Fi và IP
 */
static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data)
{
    if (event_base == WIFI_EVENT &&
        event_id == WIFI_EVENT_STA_START)
    {
        /* Wi-Fi đã khởi động, bắt đầu kết nối router */
        esp_err_t err = esp_wifi_connect();

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Khong the bat dau ket noi: %s",
                     esp_err_to_name(err));
        }
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        /* Mất kết nối hoặc kết nối thất bại */
        xEventGroupClearBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );

        ESP_LOGW(TAG, "Mat ket noi Wi-Fi, dang thu ket noi lai...");

        esp_err_t err = esp_wifi_connect();

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Loi ket noi lai: %s",
                     esp_err_to_name(err));
        }
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        /* Router đã cấp địa chỉ IP cho ESP32 */
        ip_event_got_ip_t *event =
            (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "Ket noi Wi-Fi thanh cong");
        ESP_LOGI(TAG, "Dia chi IP: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(
            wifi_event_group,
            WIFI_CONNECTED_BIT
        );
    }
}


/*
 * Khởi tạo và bắt đầu kết nối Wi-Fi
 */
esp_err_t app_wifi_init(void)
{
    if (wifi_initialized) {
        return ESP_OK;
    }

    /* Khởi tạo bộ nhớ NVS dùng bởi Wi-Fi */
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Loi khoi tao NVS: %s",
                 esp_err_to_name(err));
        return err;
    }

    /* Khởi tạo TCP/IP stack */
    err = esp_netif_init();

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    /* Tạo event loop mặc định */
    err = esp_event_loop_create_default();

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    /* Tạo giao diện Wi-Fi Station */
    esp_netif_create_default_wifi_sta();

    /* Khởi tạo driver Wi-Fi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    err = esp_wifi_init(&cfg);

    if (err != ESP_OK) {
        return err;
    }

    wifi_event_group = xEventGroupCreate();

    if (wifi_event_group == NULL) {
        return ESP_ERR_NO_MEM;
    }

    /* Đăng ký nhận sự kiện Wi-Fi */
    err = esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        &wifi_handler_instance
    );

    if (err != ESP_OK) {
        return err;
    }

    /* Đăng ký nhận sự kiện được cấp địa chỉ IP */
    err = esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        &ip_handler_instance
    );

    if (err != ESP_OK) {
        return err;
    }

    /* Cấu hình SSID và mật khẩu */
    wifi_config_t wifi_config = {0};

    snprintf((char *)wifi_config.sta.ssid,
             sizeof(wifi_config.sta.ssid),
             "%s", WIFI_SSID);

    snprintf((char *)wifi_config.sta.password,
             sizeof(wifi_config.sta.password),
             "%s", WIFI_PASSWORD);

    /* Dùng WPA2 nếu mạng yêu cầu mật khẩu */
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    /* Chuyển sang chế độ Station */
    err = esp_wifi_set_mode(WIFI_MODE_STA);

    if (err != ESP_OK) {
        return err;
    }

    /* Áp dụng cấu hình */
    err = esp_wifi_set_config(
        WIFI_IF_STA,
        &wifi_config
    );

    if (err != ESP_OK) {
        return err;
    }

    /* Khởi động Wi-Fi */
    err = esp_wifi_start();

    if (err != ESP_OK) {
        return err;
    }

    wifi_initialized = true;

    ESP_LOGI(TAG, "Da khoi dong Wi-Fi Station");

    return ESP_OK;
}


/*
 * Chờ đến khi kết nối thành công hoặc hết thời gian chờ
 */
esp_err_t app_wifi_wait_connected(TickType_t timeout_ticks)
{
    if (wifi_event_group == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdTRUE,
        timeout_ticks
    );

    if ((bits & WIFI_CONNECTED_BIT) != 0) {
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}


/*
 * Kiểm tra trạng thái kết nối
 */
bool app_wifi_is_connected(void)
{
    if (wifi_event_group == NULL) {
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(wifi_event_group);

    return (bits & WIFI_CONNECTED_BIT) != 0;
}
