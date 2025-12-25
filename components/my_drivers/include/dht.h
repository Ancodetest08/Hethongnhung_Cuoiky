#pragma once
#include "driver/gpio.h"

typedef enum { DHT_TYPE_DHT11 = 0, DHT_TYPE_DHT22 } dht_sensor_type_t;
esp_err_t dht_read_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature);