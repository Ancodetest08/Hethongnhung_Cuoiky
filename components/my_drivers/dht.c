#include "dht.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#define DHT_TIMEOUT 100

static int get_signal_duration(gpio_num_t pin, int level) {
    int count = 0;
    while (gpio_get_level(pin) != level) {
        if (count++ > DHT_TIMEOUT) return -1;
        esp_rom_delay_us(1);
    }
    int64_t start = esp_timer_get_time();
    while (gpio_get_level(pin) == level) {
        if ((esp_timer_get_time() - start) > DHT_TIMEOUT) return -1;
    }
    return (int)(esp_timer_get_time() - start);
}

esp_err_t dht_read_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature) {
    uint8_t data[5] = {0};

    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD); 
    gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    gpio_set_level(pin, 0);
    esp_rom_delay_us(20000);
    gpio_set_level(pin, 1);
    esp_rom_delay_us(30);

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    if (get_signal_duration(pin, 0) == -1) { portEXIT_CRITICAL(&mux); return ESP_FAIL; }
    if (get_signal_duration(pin, 1) == -1) { portEXIT_CRITICAL(&mux); return ESP_FAIL; }

    for (int i = 0; i < 40; i++) {
        if (get_signal_duration(pin, 0) == -1) { portEXIT_CRITICAL(&mux); return ESP_FAIL; }
        int width = get_signal_duration(pin, 1);
        if (width == -1) { portEXIT_CRITICAL(&mux); return ESP_FAIL; }
        if (width > 50) data[i/8] |= (1 << (7-(i%8)));
    }

    portEXIT_CRITICAL(&mux);

    if (data[4] != ((data[0]+data[1]+data[2]+data[3]) & 0xFF)) return ESP_FAIL;

    if (sensor_type == DHT_TYPE_DHT22) {
        *humidity = (float)((data[0] << 8) | data[1]) / 10.0;
        int16_t t = (data[2] << 8) | data[3];
        if (t & 0x8000) { t &= 0x7FFF; t = -t; }
        *temperature = (float)t / 10.0;
    } else {
        *humidity = (float)data[0];
        *temperature = (float)data[2];
    }
    return ESP_OK;
}