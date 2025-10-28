#ifndef LED_HANDLER_H
#define LED_HANDLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "wifi_manager.h"

#define WIFI_STA_STATUS_LED_PIN GPIO_NUM_2

typedef enum {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
    LED_MODE_BLINK_3X
} led_mode_t;
extern led_mode_t led_mode;

void led_init(void);

#endif