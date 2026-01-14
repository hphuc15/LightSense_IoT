#include "Button.h"

static QueueHandle_t gpio_evt_queue = NULL;
static bool button_pressed = false;
static int64_t press_start_time = 0;
static ButtonCallback_t long_press_callback = NULL;

static const gpio_config_t button_io = {
    .intr_type = GPIO_INTR_ANYEDGE,
    .mode = GPIO_MODE_INPUT,
    .pin_bit_mask = (1ULL << BUTTON_GPIO_NUM),
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .pull_up_en = GPIO_PULLUP_ENABLE
};

static void IRAM_ATTR ButtonIsr(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void ButtonTask(void *arg)
{
    uint32_t io_num;
    bool callback_triggered = false;

    while (1)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, pdMS_TO_TICKS(100)))
        {
            int level = gpio_get_level(io_num);

            if (level == 0)
            {
                if (!button_pressed)
                {
                    button_pressed = true;
                    callback_triggered = false;
                    press_start_time = esp_timer_get_time() / 1000;
                }
            }
            else
            {
                button_pressed = false;
                callback_triggered = false;
            }
        }

        if (button_pressed && !callback_triggered)
        {
            int64_t current_time = esp_timer_get_time() / 1000;
            int64_t press_duration = current_time - press_start_time;

            if (press_duration >= BUTTON_LONG_PRESS_TIME_MS)
            {
                if (long_press_callback != NULL)
                {
                    long_press_callback();
                }
                callback_triggered = true;
            }
        }
    }
}

void ButtonInit(ButtonCallback_t callback)
{
    long_press_callback = callback;
    gpio_config(&button_io);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(ButtonTask, "[ButtonTask]", 8192, NULL, 10, NULL);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO_NUM, ButtonIsr, (void*)BUTTON_GPIO_NUM);
}