
#include "lcd_i2c.h"

#include <string.h>
#include "esp_rom_sys.h"

#define LCD_RS          0x01
#define LCD_RW          0x02
#define LCD_EN          0x04
#define LCD_BACKLIGHT   0x08

#define LCD_CMD_CLEAR   0x01
#define LCD_CMD_HOME    0x02
#define LCD_CMD_ENTRY   0x06
#define LCD_CMD_DISPLAY 0x0C
#define LCD_CMD_FUNCTION 0x28
/*thực chất kiểu dữ liệu esp_err_t là một kiểu dữ liệu int, nhưng tại vì muốn cho mn thấy đây là kiểu dữ liệu dùng để
lưu mã lỗi nên mới đổi tên như vậy*/
static esp_err_t pcf8574_write(lcd_i2c_t *lcd, uint8_t data)/*hàm ghi dữ liệu có nhiệm vụ thực hiện truyền dữ liệu
từ esp32 sang pcf bằng giao thức i2c với tham số đầu vào là data và thông số của lcd cần gửi */
{
    return i2c_master_transmit(
        lcd->dev,
        &data,
        1,
        100
    );
}

/* Gửi một byte dữ liệu xuống PCF8574 */
static esp_err_t lcd_expander_write(lcd_i2c_t *lcd,
                                    uint8_t data)
{
    return pcf8574_write(
        lcd,
        data | lcd->backlight
    );
}

/* SCL giúp ESP32 truyền dữ liệu tới PCF8574. Enable giúp PCF8574 báo cho LCD biết thời điểm nhận dữ liệu*/
static esp_err_t lcd_pulse_enable(lcd_i2c_t *lcd,
                                  uint8_t data)
{
    esp_err_t ret;

    ret = lcd_expander_write(lcd, data | LCD_EN);
    if (ret != ESP_OK) return ret;

    esp_rom_delay_us(1);

    ret = lcd_expander_write(lcd, data & ~LCD_EN);
    if (ret != ESP_OK) return ret;

    esp_rom_delay_us(50);

    return ESP_OK;
}

/* Gửi 4 bit dữ liệu */
static esp_err_t lcd_write_nibble(lcd_i2c_t *lcd,
                                  uint8_t nibble,
                                  uint8_t mode)
{
    uint8_t data = (nibble & 0xF0) | mode;

    esp_err_t ret = lcd_expander_write(lcd, data);
    if (ret != ESP_OK) return ret;

    return lcd_pulse_enable(lcd, data);
}

/* Gửi một byte: command hoặc data */
static esp_err_t lcd_write_byte(lcd_i2c_t *lcd,
                                uint8_t value,
                                uint8_t mode)
{
    esp_err_t ret;

    ret = lcd_write_nibble(lcd, value & 0xF0, mode);
    if (ret != ESP_OK) return ret;

    return lcd_write_nibble(lcd, value << 4, mode);
}

/* Gửi lệnh */
static esp_err_t lcd_send_command(lcd_i2c_t *lcd,
                                  uint8_t command)
{
    esp_err_t ret = lcd_write_byte(lcd, command, 0);

    if (ret == ESP_OK) {
        if (command == LCD_CMD_CLEAR ||
            command == LCD_CMD_HOME) {
            esp_rom_delay_us(2000);
        }
    }

    return ret;
}

/* Gửi ký tự */
static esp_err_t lcd_send_data(lcd_i2c_t *lcd,
                               uint8_t data)
{
    return lcd_write_byte(lcd, data, LCD_RS);
}

/* Khởi tạo LCD */
esp_err_t lcd_i2c_init(lcd_i2c_t *lcd,
                       i2c_master_bus_handle_t bus,
                       uint8_t address)
{
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 100000,
    };

    esp_err_t ret = i2c_master_bus_add_device(
        bus,
        &dev_config,
        &lcd->dev
    );

    if (ret != ESP_OK) return ret;

    lcd->backlight = LCD_BACKLIGHT;

    esp_rom_delay_us(50000);

    /* Khởi tạo chế độ 4-bit */
    lcd_write_nibble(lcd, 0x30, 0);
    esp_rom_delay_us(5000);

    lcd_write_nibble(lcd, 0x30, 0);
    esp_rom_delay_us(150);

    lcd_write_nibble(lcd, 0x30, 0);
    esp_rom_delay_us(150);

    lcd_write_nibble(lcd, 0x20, 0);
    esp_rom_delay_us(150);

    lcd_send_command(lcd, LCD_CMD_FUNCTION);
    lcd_send_command(lcd, LCD_CMD_DISPLAY);
    lcd_send_command(lcd, LCD_CMD_CLEAR);
    lcd_send_command(lcd, LCD_CMD_ENTRY);

    return ESP_OK;
}

/* Xóa LCD */
esp_err_t lcd_clear(lcd_i2c_t *lcd)
{
    return lcd_send_command(lcd, LCD_CMD_CLEAR);
}

/* Đặt vị trí con trỏ */
esp_err_t lcd_set_cursor(lcd_i2c_t *lcd,
                         uint8_t col,
                         uint8_t row)
{
    static const uint8_t row_offsets[] = {
        0x00, 0x40
    };

    if (row > 1 || col > 15) {
        return ESP_ERR_INVALID_ARG;
    }

    return lcd_send_command(
        lcd,
        0x80 | (col + row_offsets[row])
    );
}

/* Hiển thị chuỗi */
esp_err_t lcd_print(lcd_i2c_t *lcd,
                    const char *text)
{
    if (text == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    while (*text) {
        esp_err_t ret = lcd_send_data(lcd, *text);

        if (ret != ESP_OK) return ret;

        text++;
    }

    return ESP_OK;
}

/* Điều khiển đèn nền */
esp_err_t lcd_backlight(lcd_i2c_t *lcd,
                        uint8_t enable)
{
    lcd->backlight = enable ? LCD_BACKLIGHT : 0;

    return lcd_expander_write(lcd, 0);
}
