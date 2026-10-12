
#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include "esp_err.h"
#include "dht11.h"

typedef struct {
    // DHT11
    int temperature_c;
    int humidity_pct;

    // MQ-2: gia tri ADC tho
    int mq2_raw;

    // Sharp GP2Y1010AU0F: gia tri ADC tho
    int dust_raw;

    // Trang thai doc cua tung cam bien
    esp_err_t dht11_status;
    esp_err_t mq2_status;
    esp_err_t dust_status;

} sensor_data_t;

// Khoi tao ca 3 cam bien
esp_err_t sensor_data_init(void);

// Doc va tong hop du lieu tu ca 3 cam bien
esp_err_t sensor_data_read(sensor_data_t *data);

#endif
