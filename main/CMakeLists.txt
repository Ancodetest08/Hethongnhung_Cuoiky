idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES my_common my_drivers my_network nvs_flash driver esp_event)