#ifndef BUTTON_H
#define BUTTON_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define BUTTON_GPIO_NUM GPIO_NUM_16
#define BUTTON_LONG_PRESS_TIME_MS 3000

typedef void (*ButtonCallback_t)(void);

void ButtonInit(ButtonCallback_t callback);

#endif