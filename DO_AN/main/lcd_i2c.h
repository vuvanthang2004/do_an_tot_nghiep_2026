
#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

typedef struct {
    i2c_master_dev_handle_t dev;
    uint8_t backlight;
} lcd_i2c_t;

esp_err_t lcd_i2c_init(lcd_i2c_t *lcd,
                       i2c_master_bus_handle_t bus,
                       uint8_t address);

esp_err_t lcd_clear(lcd_i2c_t *lcd);

esp_err_t lcd_set_cursor(lcd_i2c_t *lcd,
                         uint8_t col,
                         uint8_t row);

esp_err_t lcd_print(lcd_i2c_t *lcd,
                    const char *text);

esp_err_t lcd_backlight(lcd_i2c_t *lcd,
                        uint8_t enable);

#endif
