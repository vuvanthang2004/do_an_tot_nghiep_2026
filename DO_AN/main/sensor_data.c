
#include "sensor_data.h"

#include "mq_2.h"
#include "Sharp_GP2Y1010AU0F.h"

#include <stddef.h>

// Khoi tao ca 3 cam bien
esp_err_t sensor_data_init(void)
{
    esp_err_t first_error = ESP_OK;
    esp_err_t ret;

    // Khoi tao DHT11
    ret = dht11_init();
    if (ret != ESP_OK && first_error == ESP_OK) {
        first_error = ret;
    }

    // Khoi tao MQ-2
    ret = mq2_init();
    if (ret != ESP_OK && first_error == ESP_OK) {
        first_error = ret;
    }

    // Khoi tao Sharp GP2Y1010AU0F
    ret = sharp_gp2y1010_init();
    if (ret != ESP_OK && first_error == ESP_OK) {
        first_error = ret;
    }

    return first_error;
}

// Doc va tong hop du lieu
esp_err_t sensor_data_read(sensor_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Khoi tao gia tri ban dau
    *data = (sensor_data_t){0};

    esp_err_t first_error = ESP_OK;
    esp_err_t ret;

    // 1. Doc DHT11
    dht11_data_t dht_data = {0};

    ret = dht11_read(&dht_data);
    data->dht11_status = ret;

    if (ret == ESP_OK) {
        data->temperature_c = dht_data.temperature_c;
        data->humidity_pct = dht_data.humidity_pct;
    } else {
        first_error = ret;
    }

    // 2. Doc MQ-2
    int mq2_value = 0;

    ret = mq2_read_raw(&mq2_value);
    data->mq2_status = ret;

    if (ret == ESP_OK) {
        data->mq2_raw = mq2_value;
    } else if (first_error == ESP_OK) {
        first_error = ret;
    }

    // 3. Doc Sharp GP2Y1010AU0F
    int dust_value = 0;

    ret = sharp_gp2y1010_read_raw(&dust_value);
    data->dust_status = ret;

    if (ret == ESP_OK) {
        data->dust_raw = dust_value;
    } else if (first_error == ESP_OK) {
        first_error = ret;
    }

    return first_error;
}
