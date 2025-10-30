#include "wifi_manager.h"
#include "bh1750_sensor.h"
#include "button_handler.h"
#include "led_handler.h"
#include "http_client.h"

post_data_t post_data = {0};
char bh1750_json_data[128];

void app_main(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());                // Initialize networking stack
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Create default event loop needed by the  main app
    ESP_ERROR_CHECK(nvs_flash_init());                // Initialize NVS needed by Wi-Fi
    wifi_manager_init_sta(&post_data.sta_info);

    http_client_t client;
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, ESP_WIFI_STA_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & ESP_WIFI_STA_CONNECTED_BIT)
    {
        ESP_LOGI("APP", "Wi-Fi connected, initializing HTTP client...");
        http_client_init(&client, "192.168.1.5", 5000);
    }

    bh1750_init(); // Initialize BH1750 Sensor
    button_init();
    led_init();

    while (1)
    {
        bh1750_get_json_string(bh1750_handle, &bh1750_data, bh1750_json_data);
        ESP_LOGI("BH1750", "Data: %s", bh1750_json_data);
        http_client_post(&client, "/test_post", bh1750_json_data);
        bh1750_json_data[0] = '\0';
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}