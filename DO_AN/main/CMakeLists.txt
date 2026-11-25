"""
file này cho biết file c nào được biên dịch
thư viện nào cần fil.h 
thư viện esp-idf cần dùng
quản lý các thư viện tự tạo cũng như thư viện của esp-idf
nếu không báo các thư viện cần dùng vào trong đây thì ko biên dịch được
"""
idf_component_register(SRCS "lcd_i2c.c" "app_control.c" "app_device.c" "app_mqtt.c" "app_wifi.c" "sensor_process.c" "sensor_data.c" "Sharp_GP2Y1010AU0F.c" "dht11.c" "mq_2.c" "main.c"
                    INCLUDE_DIRS "."
                    
REQUIRES esp_wifi esp_event esp_netif nvs_flash log freertos
             driver esp_adc esp_timer mqtt  json                  
                    )