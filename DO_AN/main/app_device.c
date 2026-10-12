#include "app_device.h"

#include "driver/gpio.h"

/* GPIO dieu khien transistor va relay */
#define FAN_RELAY_GPIO    GPIO_NUM_15
#define TEC_RELAY_GPIO    GPIO_NUM_18
#define ION_RELAY_GPIO    GPIO_NUM_19

/*
 * Mac dinh:
 * HIGH = relay ON
 * LOW  = relay OFF
 *
 * Chi dung cau hinh nay neu mach transistor cua ban
 * duoc thiet ke de GPIO HIGH kich relay.
 */
#define RELAY_ACTIVE_LEVEL    1
#define RELAY_INACTIVE_LEVEL  0

/* Ham dieu khien GPIO dung chung */
static esp_err_t relay_set_state(gpio_num_t gpio, bool on)
{
    int level = on ? RELAY_ACTIVE_LEVEL : RELAY_INACTIVE_LEVEL;

    return gpio_set_level(gpio, level);
}

/* Khoi tao GPIO */
esp_err_t app_device_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask =
            (1ULL << FAN_RELAY_GPIO) |
            (1ULL << TEC_RELAY_GPIO) |
            (1ULL << ION_RELAY_GPIO),

        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);

    if (ret != ESP_OK) {
        return ret;
    }

    /* Dat cac relay ve trang thai OFF ban dau */
    ret = gpio_set_level(FAN_RELAY_GPIO, RELAY_INACTIVE_LEVEL);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = gpio_set_level(TEC_RELAY_GPIO, RELAY_INACTIVE_LEVEL);
    if (ret != ESP_OK) {
        return ret;
    }

    ret = gpio_set_level(ION_RELAY_GPIO, RELAY_INACTIVE_LEVEL);
    if (ret != ESP_OK) {
        return ret;
    }

    return ESP_OK;
}

/* Bat/tat quat FAN */
esp_err_t fan_set_state(bool on)
{
    return relay_set_state(FAN_RELAY_GPIO, on);
}

/* Bat/tat thiet bi lam mat TEC */
esp_err_t tec_set_state(bool on)
{
    return relay_set_state(TEC_RELAY_GPIO, on);
}

/* Bat/tat thiet bi tao ion am ION */
esp_err_t ion_set_state(bool on)
{
    return relay_set_state(ION_RELAY_GPIO, on);
}