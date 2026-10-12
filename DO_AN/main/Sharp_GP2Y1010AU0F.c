#include "Sharp_GP2Y1010AU0F.h"

#include <stdint.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_rom_sys.h"

// ==================== CAU HINH ====================

#define SHARP_VO_GPIO           GPIO_NUM_35
#define SHARP_LED_GPIO          GPIO_NUM_25

#define SHARP_ADC_UNIT          ADC_UNIT_1
#define SHARP_ADC_CHANNEL       ADC_CHANNEL_7
#define SHARP_ADC_ATTEN         ADC_ATTEN_DB_12

#define SHARP_SAMPLE_COUNT      10

// Chu ky dieu khien LED theo datasheet
#define SHARP_LED_ON_US         280
#define SHARP_ADC_DELAY_US      40
#define SHARP_LED_OFF_WAIT_US   9680

// ==================== BIEN NOI BO ====================

static adc_oneshot_unit_handle_t adc_handle = NULL;
static bool sharp_initialized = false;

// ==================== KHOI TAO ====================

esp_err_t sharp_gp2y1010_init(void)
{
    esp_err_t err;

    // 1. Cau hinh GPIO25 dieu khien LED
    gpio_config_t led_config = {
        .pin_bit_mask = 1ULL << SHARP_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    err = gpio_config(&led_config);
    if (err != ESP_OK) {
        return err;
    }

    // Mac dinh tat LED
    err = gpio_set_level(SHARP_LED_GPIO, 0);
    if (err != ESP_OK) {
        return err;
    }

    // 2. Khoi tao ADC1
    adc_oneshot_unit_init_cfg_t adc_init_config = {
        .unit_id = SHARP_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    err = adc_oneshot_new_unit(
        &adc_init_config,
        &adc_handle
    );

    if (err != ESP_OK) {
        return err;
    }

    // 3. Cau hinh kenh ADC GPIO35
    adc_oneshot_chan_cfg_t adc_channel_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = SHARP_ADC_ATTEN,
    };

    err = adc_oneshot_config_channel(
        adc_handle,
        SHARP_ADC_CHANNEL,
        &adc_channel_config
    );

    if (err != ESP_OK) {
        adc_oneshot_del_unit(adc_handle);
        adc_handle = NULL;
        return err;
    }

    sharp_initialized = true;

    return ESP_OK;
}

// ==================== DOC MOT MAU ADC ====================

static esp_err_t sharp_read_adc_raw(int *raw)
{
    if (raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!sharp_initialized || adc_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err;

    // Bat LED hong ngoai
    err = gpio_set_level(SHARP_LED_GPIO, 1);
    if (err != ESP_OK) {
        return err;
    }

    // Cho LED va mach cam bien on dinh
    esp_rom_delay_us(SHARP_LED_ON_US);

    // Lay mau ADC tai VO
    esp_rom_delay_us(SHARP_ADC_DELAY_US);

    err = adc_oneshot_read(
        adc_handle,
        SHARP_ADC_CHANNEL,
        raw
    );

    // Tat LED sau khi lay mau
    esp_err_t led_err = gpio_set_level(
        SHARP_LED_GPIO,
        0
    );

    // Cho het chu ky lay mau
    esp_rom_delay_us(SHARP_LED_OFF_WAIT_US);

    if (err != ESP_OK) {
        return err;
    }

    if (led_err != ESP_OK) {
        return led_err;
    }

    return ESP_OK;
}

// ==================== DOC GIA TRI TRUNG BINH ====================

esp_err_t sharp_gp2y1010_read_raw(int *raw_average)
{
    if (raw_average == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int sum = 0;

    for (int i = 0; i < SHARP_SAMPLE_COUNT; i++) {
        int raw = 0;

        esp_err_t err = sharp_read_adc_raw(&raw);

        if (err != ESP_OK) {
            return err;
        }

        sum += raw;
    }

    *raw_average = sum / SHARP_SAMPLE_COUNT;

    return ESP_OK;
}