#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include <stdbool.h>

#define WIFI_CONNECTED_BIT  (1 << 0)

typedef enum {
    CMD_TOGGLE_MODE,
    CMD_CHANGE_SPEED,
    CMD_UPDATE_TEMP,
    CMD_WEB_SET_SPEED,
    CMD_WEB_SET_MODE,
    CMD_WEB_SET_THRESH,
    CMD_WEB_SET_TIMER,
    CMD_TIMER_EXPIRED
} command_type_t;

typedef struct {
    command_type_t type;
    int value;
    float f_value;
} app_command_t;

typedef struct {
    bool is_auto;
    int speed_level;
    float current_temp;
    float current_hum;
    float thresh_1, thresh_2, thresh_3;
    int timer_minutes;
    bool timer_active;
} system_state_t;

extern QueueHandle_t xQueueCmd;
extern SemaphoreHandle_t xMutexState;
extern EventGroupHandle_t xEventGroupSys;
extern TimerHandle_t xTimerShutdown;
extern TaskHandle_t xTaskLedHandle;
extern system_state_t g_state;

void app_system_init(void);
