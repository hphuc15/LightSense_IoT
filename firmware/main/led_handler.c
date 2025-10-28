#include "led_handler.h"

led_mode_t led_mode = LED_MODE_OFF;

void led_task(void *arg)
{
    while (1)
    {
        switch (led_mode)
        {
        case LED_MODE_OFF:
            gpio_set_level(WIFI_STA_STATUS_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;

        case LED_MODE_ON:
            gpio_set_level(WIFI_STA_STATUS_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            break;

        case LED_MODE_BLINK:
            gpio_set_level(WIFI_STA_STATUS_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(300));
            gpio_set_level(WIFI_STA_STATUS_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(300));
            break;

        case LED_MODE_BLINK_3X:
            for (int i = 0; i < 3; i++)
            {
                gpio_set_level(WIFI_STA_STATUS_LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(200));
                gpio_set_level(WIFI_STA_STATUS_LED_PIN, 0);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            led_mode = LED_MODE_OFF; // xong 3 lần thì dừng
            break;
        }
    }
}

void wifi_led_task(void *arg)
{
    EventGroupHandle_t wifi_event_group = (EventGroupHandle_t)arg;
    while (1)
    {
        EventBits_t bits = xEventGroupGetBits(wifi_event_group);
        if (bits & ESP_WIFI_STA_CONNECTED_BIT)
        {
            led_mode = LED_MODE_ON;
        }
        else if (bits & ESP_WIFI_STA_FAIL_BIT)
        {
            led_mode = LED_MODE_OFF;
        }
        else if (bits & ESP_WIFI_AP_ACTIVE_BIT)
        {
            led_mode = LED_MODE_BLINK;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void led_init(void)
{
    gpio_config_t led_io_conf = {
        .pin_bit_mask = 1ULL << WIFI_STA_STATUS_LED_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_io_conf);

    xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);
    xTaskCreate(wifi_led_task, "wifi_led_task", 2048, s_wifi_event_group, 5, NULL);
}