idf_component_register(SRCS "app_wifi.c" "app_webserver.c"
                       INCLUDE_DIRS "include"
                       REQUIRES my_common esp_http_server esp_wifi lwip json)