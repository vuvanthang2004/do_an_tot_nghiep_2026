
#ifndef SENSOR_PROCESS_H
#define SENSOR_PROCESS_H

#include <stdbool.h>
#include "esp_err.h"
#include "sensor_data.h"

typedef struct {
    sensor_data_t data;

    bool temperature_valid;
    bool humidity_valid;
    bool mq2_valid;
    bool dust_valid;

} sensor_processed_data_t;

// Kiem tra va xu ly du lieu cam bien
esp_err_t sensor_process_data(
    const sensor_data_t *raw,
    sensor_processed_data_t *processed
);

#endif
