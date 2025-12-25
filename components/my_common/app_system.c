#include "app_system.h"

QueueHandle_t xQueueCmd = NULL;
SemaphoreHandle_t xMutexState = NULL;
EventGroupHandle_t xEventGroupSys = NULL;
TimerHandle_t xTimerShutdown = NULL;
TaskHandle_t xTaskLedHandle = NULL;

system_state_t g_state = { 
    .is_auto = true, 
    .speed_level = 0, 
    .current_temp = 0.0,
    .current_hum = 0.0,
    .thresh_1 = 27.0, .thresh_2 = 29.0, .thresh_3 = 32.0,
    .timer_minutes = 0,
    .timer_active = false
};

void vTimerCallback(TimerHandle_t xTimer) {
    app_command_t cmd = { .type = CMD_TIMER_EXPIRED };
    xQueueSend(xQueueCmd, &cmd, 0);
}

void app_system_init(void) {
    xQueueCmd = xQueueCreate(10, sizeof(app_command_t));
    xMutexState = xSemaphoreCreateMutex();
    xEventGroupSys = xEventGroupCreate();
    xTimerShutdown = xTimerCreate("ShutdownTimer", pdMS_TO_TICKS(60000), pdFALSE, (void*)0, vTimerCallback);
}
