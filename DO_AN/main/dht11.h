#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t humidity_pct;    // độ ẩm (%), phần nguyên
    uint8_t temperature_c;   // nhiệt độ (°C), phần nguyên
} dht11_data_t;

esp_err_t dht11_init(void);
esp_err_t dht11_read(dht11_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* DHT11_H */