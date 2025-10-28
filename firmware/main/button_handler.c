// Line 22
#include "button_handler.h"

volatile bool button_pressed = false;
volatile TickType_t press_start_time = 0;
bool action_triggered = false;

void IRAM_ATTR button_isr_handler(void *arg)
{
    int state = gpio_get_level(WIFI_CONF_BUTT_PIN);
    if (state == 0)
    {
        press_start_time = xTaskGetTickCountFromISR();
        button_pressed = true;
    }
    else
    {
        button_pressed = false;
    }
}

void button_long_press_callback(void)
{
    ESP_LOGI("BUTTON", "Button long pressed 3s: execute: execute action");
    //wifi_manager_stop();
    vTaskDelay(pdMS_TO_TICKS(50));
    wifi_manager_init_softap();
}

void button_task(void *arg)
{
    while (1)
    {
        if (button_pressed)
        {
            TickType_t press_time = xTaskGetTickCount() - press_start_time;
            if (press_time >= pdMS_TO_TICKS(WIFI_CONF_BUTT_HOLD_TIME_MS) && !action_triggered)
            {
                action_triggered = true;
                button_long_press_callback();
            }
            else
            {
                action_triggered = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void button_init()
{
    gpio_config_t butt_io_cfg = {
        .pin_bit_mask = 1ULL << WIFI_CONF_BUTT_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE};
    gpio_config(&butt_io_cfg);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(WIFI_CONF_BUTT_PIN, button_isr_handler, NULL);

    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
}
