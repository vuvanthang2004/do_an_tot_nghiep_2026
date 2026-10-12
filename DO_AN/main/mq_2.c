#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "mq_2.h"

static const char *TAG ="MQ_2";
#define MQ2_ADC_UNIT ADC_UNIT_1
#define MQ2_ADC_CHANNEL ADC_CHANNEL_5
#define MQ2_ADC_ATTEN ADC_ATTEN_DB_12

static adc_oneshot_unit_handle_t adc_handle =NULL;
static adc_cali_handle_t cali_handle=NULL;
static bool calibration_enabled=false;

esp_err_t mq2_init(void){
adc_oneshot_unit_init_cfg_t init_config={
    .unit_id= MQ2_ADC_UNIT, 
};
esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);

if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Khong khoi tao duoc ADC1");
        return ret;
    }
adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = MQ2_ADC_ATTEN,
    };
ret = adc_oneshot_config_channel(
        adc_handle,
        MQ2_ADC_CHANNEL,
        &channel_config
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Khong cau hinh duoc ADC channel");
        return ret;
    }
     adc_cali_line_fitting_config_t cali_config = {
        .unit_id = MQ2_ADC_UNIT,
        .atten = MQ2_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    ret = adc_cali_create_scheme_line_fitting(
        &cali_config, &cali_handle
    );

    if (ret == ESP_OK) {
        calibration_enabled = true;
        ESP_LOGI(TAG, "Da khoi tao hieu chuan ADC");
    } else {
        calibration_enabled = false;
        ESP_LOGW(TAG, "Khong co hieu chuan ADC, van doc duoc ADC raw");
    }

    ESP_LOGI(TAG, "MQ-2 khoi tao thanh cong tren GPIO33");
    return ESP_OK;
}

esp_err_t mq2_read_raw(int *raw)
{
    if (raw == NULL || adc_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return adc_oneshot_read(
        adc_handle,
        MQ2_ADC_CHANNEL,
        raw
    );
}

esp_err_t mq2_read_voltage(int *millivolts)
{
    if (millivolts == NULL || adc_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw = 0;
    esp_err_t ret = mq2_read_raw(&raw);

    if (ret != ESP_OK) {
        return ret;
    }

    if (!calibration_enabled) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    return adc_cali_raw_to_voltage(
        cali_handle,
        raw,
        millivolts
    );
}

