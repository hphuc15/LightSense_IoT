#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "wifi_manager.h"

#define WIFI_CONF_BUTT_PIN GPIO_NUM_16
#define WIFI_CONF_BUTT_HOLD_TIME_MS 3000

extern volatile bool button_pressed;
extern volatile TickType_t press_start_time;
extern bool action_triggered;


void button_init();



#endif