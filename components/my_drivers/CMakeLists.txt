idf_component_register(SRCS "app_driver.c" "dht.c"
                       INCLUDE_DIRS "include"
                       REQUIRES led_strip my_common driver esp_timer)