#include "wifi_manager.h"
#include "bh1750_sensor.h"
#include "button_handler.h"
#include "led_handler.h"

// Struct lưu config cho STA mode và Server IP
sta_info_t sta_info;
// Lưu IP của Server chính khi được nhận từ captive portal
char main_server_ip[40];

void app_main(void)
{
    // Initialize BH1750 Sensor
    bh1750_init();
    

    // Cấu hình module wifi (button, status led)
    s_wifi_event_group = xEventGroupCreate();

    button_init();
    led_init();

    // Initialize networking stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop needed by the  main app
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize NVS needed by Wi-Fi
    ESP_ERROR_CHECK(nvs_flash_init());


    // test null nvs:
    // wifi_manager_nvs_erase_config();

    // wifi_manager_init_softap();
    wifi_manager_init_sta(&sta_info);
    // Configure DNS-based captive portal, if configured


    while(1){
        bh1750_get_json_string(bh1750_handle, &bh1750_data, bh1750_json_data);
        ESP_LOGI("BH1750", "Data: %s", bh1750_json_data);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

}