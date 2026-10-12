
#include "sensor_process.h"

#include <string.h>

#define ADC_MAX_VALUE       4095
#define FILTER_WINDOW_SIZE  5

typedef struct {
    int samples[FILTER_WINDOW_SIZE];
    int count;
    int index;
} adc_filter_t;

static adc_filter_t mq2_filter = {0};
static adc_filter_t dust_filter = {0};

// Them mau moi va tinh trung binh cac mau da co
static int adc_filter_update(adc_filter_t *filter, int value)
{
    filter->samples[filter->index] = value;

    filter->index =
        (filter->index + 1) % FILTER_WINDOW_SIZE;

    if (filter->count < FILTER_WINDOW_SIZE) {
        filter->count++;
    }

    int sum = 0;

    for (int i = 0; i < filter->count; i++) {
        sum += filter->samples[i];
    }

    return sum / filter->count;
}

esp_err_t sensor_process_data(
    const sensor_data_t *raw,
    sensor_processed_data_t *processed)
{
    if (raw == NULL || processed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Sao chep du lieu dau vao
    memset(processed, 0, sizeof(*processed));
    processed->data = *raw;

    // 1. Kiem tra DHT11
    if (raw->dht11_status == ESP_OK) {
        if (raw->temperature_c >= 0 &&
            raw->temperature_c <= 50) {
            processed->temperature_valid = true;
        }

        if (raw->humidity_pct >= 20 &&
            raw->humidity_pct <= 90) {
            processed->humidity_valid = true;
        }
    }

    // 2. Kiem tra va loc MQ-2
    if (raw->mq2_status == ESP_OK &&
        raw->mq2_raw >= 0 &&
        raw->mq2_raw <= ADC_MAX_VALUE) {

        processed->mq2_valid = true;

        processed->data.mq2_raw =
            adc_filter_update(&mq2_filter, raw->mq2_raw);
    }

    // 3. Kiem tra va loc cam bien bui
    if (raw->dust_status == ESP_OK &&
        raw->dust_raw >= 0 &&
        raw->dust_raw <= ADC_MAX_VALUE) {

        processed->dust_valid = true;

        processed->data.dust_raw =
            adc_filter_update(&dust_filter, raw->dust_raw);
    }

    return ESP_OK;
}
