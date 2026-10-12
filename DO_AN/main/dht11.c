
#include "dht11.h"

#include <stdint.h>
#include <stddef.h>

#include "driver/gpio.h"
#include "esp_rom_sys.h"

// ==================== CAU HINH ====================

#define DHT11_GPIO          GPIO_NUM_27
#define DHT11_TIMEOUT_US    100

// ==================== CHO MUC GPIO ====================

static esp_err_t dht11_wait_level(int level)
{
    int elapsed = 0;

    while (gpio_get_level(DHT11_GPIO) != level) {
        if (elapsed >= DHT11_TIMEOUT_US) {
            return ESP_ERR_TIMEOUT;
        }

        esp_rom_delay_us(1);
        elapsed++;
    }

    return ESP_OK;
}

// ==================== DOC 1 BYTE ====================

static esp_err_t dht11_read_byte(uint8_t *value)
{
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t result = 0;

    for (int i = 0; i < 8; i++) {
        // Cho tin hieu bat dau bit
        esp_err_t err = dht11_wait_level(0);

        if (err != ESP_OK) {
            return err;
        }

        // Cho xung cao
        err = dht11_wait_level(1);

        if (err != ESP_OK) {
            return err;
        }

        // Do thoi gian muc cao
        int high_time = 0;

        while (gpio_get_level(DHT11_GPIO) == 1) {
            if (high_time >= DHT11_TIMEOUT_US) {
                return ESP_ERR_TIMEOUT;
            }

            esp_rom_delay_us(1);
            high_time++;
        }

        // Dich bit sang trai de tao byte
        result <<= 1;

        // Xung cao dai hon khoang 40 us duoc xem la bit 1
        if (high_time > 40) {
            result |= 1;
        }
    }

    *value = result;

    return ESP_OK;
}

// ==================== KHOI TAO DHT11 ====================

esp_err_t dht11_init(void)
{
    gpio_config_t io_config = {
        .pin_bit_mask = 1ULL << DHT11_GPIO,
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_config);

    if (err != ESP_OK) {
        return err;
    }

    // Tha duong DATA len muc cao
    return gpio_set_level(DHT11_GPIO, 1);
}

// ==================== DOC DU LIEU DHT11 ====================

esp_err_t dht11_read(dht11_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t bytes[5] = {0};

    esp_err_t err;

    // 1. ESP32 keo DATA xuong thap de bat dau giao tiep
    err = gpio_set_level(DHT11_GPIO, 0);

    if (err != ESP_OK) {
        return err;
    }

    // DHT11 can xung start toi thieu khoang 18 ms
    esp_rom_delay_us(20000);

    // 2. Tha DATA len cao
    err = gpio_set_level(DHT11_GPIO, 1);

    if (err != ESP_OK) {
        return err;
    }

    esp_rom_delay_us(30);

    // 3. Chuyen GPIO sang input de nhan du lieu
    err = gpio_set_direction(
        DHT11_GPIO,
        GPIO_MODE_INPUT
    );

    if (err != ESP_OK) {
        goto cleanup;
    }

    // 4. Cho tin hieu phan hoi cua DHT11
    // DHT11 phan hoi bang muc thap, sau do muc cao,
    // roi bat dau truyen cac bit du lieu.
    err = dht11_wait_level(0);

    if (err != ESP_OK) {
        goto cleanup;
    }

    err = dht11_wait_level(1);

    if (err != ESP_OK) {
        goto cleanup;
    }

    err = dht11_wait_level(0);

    if (err != ESP_OK) {
        goto cleanup;
    }

    // 5. Doc 5 byte = 40 bit
    for (int i = 0; i < 5; i++) {
        err = dht11_read_byte(&bytes[i]);

        if (err != ESP_OK) {
            goto cleanup;
        }
    }

    // 6. Kiem tra checksum
    uint8_t checksum = (uint8_t)(
        bytes[0] +
        bytes[1] +
        bytes[2] +
        bytes[3]
    );

    if (checksum != bytes[4]) {
        err = ESP_ERR_INVALID_CRC;
        goto cleanup;
    }

    // 7. Lay phan nguyen cua do am va nhiet do
    data->humidity_pct = bytes[0];
    data->temperature_c = bytes[2];

    err = ESP_OK;

cleanup:
    // Dua GPIO ve open-drain output va tha DATA len cao
    gpio_set_direction(
        DHT11_GPIO,
        GPIO_MODE_INPUT_OUTPUT_OD
    );

    gpio_set_level(DHT11_GPIO, 1);

    return err;
}
