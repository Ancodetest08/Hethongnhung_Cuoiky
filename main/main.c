#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "app_config.h"
#include "app_system.h"
#include "app_driver.h"
#include "dht.h" 
#include "app_wifi.h"
#include "app_webserver.h"

static const char *TAG = "MAIN";

void app_webserver_init(void);

// --- TASK 1: BUTTON (Nút nhấn) ---
void vTaskButton(void *p) {
    int last = 1; TickType_t start = 0; bool hold = false;
    while(1) {
        int cur = gpio_get_level(PIN_BUTTON);
        if (last == 1 && cur == 0) { 
            vTaskDelay(pdMS_TO_TICKS(20)); 
            if(!gpio_get_level(PIN_BUTTON)) { start=xTaskGetTickCount(); hold=false; } 
        }
        if (cur == 0 && !hold && (xTaskGetTickCount()-start > pdMS_TO_TICKS(1500))) {
            app_command_t cmd = { .type = CMD_TOGGLE_MODE }; xQueueSend(xQueueCmd, &cmd, 0); hold=true;
        }
        if (last == 0 && cur == 1 && !hold) {
            app_command_t cmd = { .type = CMD_CHANGE_SPEED }; xQueueSend(xQueueCmd, &cmd, 0);
        }
        last = cur; vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// --- TASK 2: SENSOR (Cảm biến) ---
void vTaskSensor(void *p) {
    float t=0, h=0; vTaskDelay(pdMS_TO_TICKS(2000));
    while(1) {
        if (dht_read_data(DHT_TYPE_DHT22, PIN_DHT, &h, &t) == ESP_OK) {
            app_command_t cmd = { .type = CMD_UPDATE_TEMP, .f_value = t, .value = (int)h };
            xQueueSend(xQueueCmd, &cmd, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

// --- TASK 3: LED (Dùng Task Notification) ---
void vTaskLed(void *p) {
    int bri=100, fade=2, r=0, g=0, b=0;
    while(1) {
        if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
            led_set_color(0, 0, 0); vTaskDelay(pdMS_TO_TICKS(50));
        }

        bool auto_mode = true; int spd = 0;

        if(xSemaphoreTake(xMutexState, portMAX_DELAY)) { auto_mode=g_state.is_auto; spd=g_state.speed_level; xSemaphoreGive(xMutexState); }
        
        if (spd==1) { r=0; g=255; b=0; }
        else if (spd==2) { r=255; g=255; b=0; }
        else if (spd==3) { r=255; g=0; b=0; }
        else { r = auto_mode ? 50 : 0; g = auto_mode ? 50 : 0; b = auto_mode ? 50 : 0; }

        if (auto_mode) {
            bri += fade; if(bri<=5) fade=2; if(bri>=100) fade=-2;
            if (spd == 0) { r=50; g=50; b=50; }
        } else { bri=100; fade=2; }
        
        led_set_color(r*bri/100, g*bri/100, b*bri/100);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// --- TASK 4: CONTROL (Bộ não trung tâm) ---
void vTaskControl(void *p) {
    app_command_t cmd;
    while(1) {
        if(xQueueReceive(xQueueCmd, &cmd, portMAX_DELAY)) {
            xSemaphoreTake(xMutexState, portMAX_DELAY);
            switch(cmd.type) {
                case CMD_TOGGLE_MODE: case CMD_WEB_SET_MODE:
                    g_state.is_auto = !g_state.is_auto; 
                    xTaskNotifyGive(xTaskLedHandle);
                    break;

                case CMD_CHANGE_SPEED:
                    if(!g_state.is_auto) { 
                        g_state.speed_level++; if(g_state.speed_level>3) g_state.speed_level=0; 
                        fan_set_speed(g_state.speed_level); 
                        xTaskNotifyGive(xTaskLedHandle);
                    } 
                    break;

                case CMD_WEB_SET_SPEED:
                    if(g_state.is_auto) g_state.is_auto = false; 
                    g_state.speed_level=cmd.value; 
                    fan_set_speed(g_state.speed_level); 
                    xTaskNotifyGive(xTaskLedHandle);
                    break;

                case CMD_UPDATE_TEMP:
                    g_state.current_temp = cmd.f_value;
                    g_state.current_hum = (float)cmd.value;
                    if(g_state.is_auto) {
                        int ns = 0;

                        if(g_state.current_temp > g_state.thresh_3) ns=3;
                        else if(g_state.current_temp > g_state.thresh_2) ns=2;
                        else if(g_state.current_temp > g_state.thresh_1) ns=1;

                        if(g_state.speed_level != ns) {
                            g_state.speed_level=ns; 
                            fan_set_speed(ns); 
                            xTaskNotifyGive(xTaskLedHandle);
                        }
                    } 
                    break;

                case CMD_WEB_SET_THRESH:
                    if(cmd.value == 1) g_state.thresh_1 = cmd.f_value;
                    if(cmd.value == 2) g_state.thresh_2 = cmd.f_value;
                    if(cmd.value == 3) g_state.thresh_3 = cmd.f_value;
                    break;

                case CMD_WEB_SET_TIMER:
                    if (cmd.value > 0) {
                        g_state.timer_minutes = cmd.value;
                        g_state.timer_active = true;
                        xTimerChangePeriod(xTimerShutdown, pdMS_TO_TICKS(cmd.value * 60000), 0);
                        xTimerStart(xTimerShutdown, 0);
                        ESP_LOGI(TAG, "Timer Countdown POWER OFF Started: %d mins", cmd.value);
                    } else {
                        g_state.timer_minutes = 0;
                        g_state.timer_active = false;
                        xTimerStop(xTimerShutdown, 0);
                        ESP_LOGI(TAG, "Timer Countdown POWER OFF Cancelled");
                    }
                    break;

                case CMD_TIMER_EXPIRED:
                    ESP_LOGI(TAG, "Timer Expired! Shutting down.");
                    g_state.timer_minutes = 0;
                    g_state.timer_active = false;
                    g_state.is_auto = false;
                    g_state.speed_level = 0;
                    fan_set_speed(0);
                    xTaskNotifyGive(xTaskLedHandle);
                    break;
            }
            xSemaphoreGive(xMutexState);
        }
    }
}

// --- TASK 5: MONITOR (Dùng Event Group) ---
void vTaskMonitor(void *p) {
    ESP_LOGI("MONITOR", "Waiting for WiFi...");
    xEventGroupWaitBits(xEventGroupSys, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI("MONITOR", "WiFi Connected! System Ready.");
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) { ESP_ERROR_CHECK(nvs_flash_erase()); ret = nvs_flash_init(); }
    ESP_ERROR_CHECK(ret);

    app_system_init();
    app_driver_init();
    app_wifi_init();
    app_webserver_init();

    xTaskCreate(vTaskControl, "Ctrl", 4096, NULL, 5, NULL);
    xTaskCreate(vTaskButton, "Btn", 2048, NULL, 6, NULL);
    xTaskCreate(vTaskSensor, "Sens", 2048, NULL, 4, NULL);
    xTaskCreate(vTaskLed, "Led", 2048, NULL, 5, &xTaskLedHandle);
    xTaskCreate(vTaskMonitor, "Mon", 2048, NULL, 4, NULL);
    
    ESP_LOGI(TAG, "Full FreeRTOS Features Started!");
}