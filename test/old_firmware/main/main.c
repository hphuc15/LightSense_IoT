#include "wifi_manager.h"
#include "bh1750_sensor.h"
#include "button_handler.h"
#include "led_handler.h"
#include "http_client.h"

post_data_t post_data = {0};
char bh1750_json_data[128];

// Vấn đề: client này biến toàn cục, chưa nghĩ ra phương án khác để xử lí chỗ deinit lúc press
// button quá 3s. Nôm na là button_handler.c sẽ là module cá nhân hóa chứ không tái sử dụng được,
// có thể phải dủng biến toàn cục này để truyền vào hàm xử lí khi button pressed quá 3s, nó sẽ phải
// cleanup cái client này để tái config thông qua captive portal.
http_client_t client;

void app_main(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());                // Initialize networking stack
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Create default event loop needed by the  main app
    ESP_ERROR_CHECK(nvs_flash_init());                // Initialize NVS needed by Wi-Fi
    wifi_manager_init_sta(&post_data.sta_info);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, ESP_WIFI_STA_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & ESP_WIFI_STA_CONNECTED_BIT)
    {
        ESP_LOGI("APP", "Wi-Fi connected, initializing HTTP client...");
        http_client_init(&client, post_data.server_ip, 5000);
    }

    bh1750_init(); // Initialize BH1750 Sensor
    button_init();
    led_init();

    while (1)
    {
        bh1750_get_json_string(bh1750_handle, &bh1750_data, bh1750_json_data);
        if (wifi_manager_is_sta_connected())
        {
            http_client_post(&client, "api/data/postData", bh1750_json_data); // "api/data/postData" là path trong drogon còn Flask là "/data"
            ESP_LOGI("BH1750", "Data: %s", bh1750_json_data);
        }
        bh1750_json_data[0] = '\0';
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}