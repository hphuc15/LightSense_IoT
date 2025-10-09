// Modular Programing
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"

#define ESP_WIFI_SSID "B9 106"
#define ESP_WIFI_PSW "B91062005@"
#define ESP_WIFI_MAX_RETRY 3
#define ESP_WIFI_FAIL_BIT BIT0
#define ESP_WIFI_CONNECTED_BIT BIT1

static const char *TAG_w = "wifi";
static int s_retry_num = 0;

EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG_w, "WiFi started, connecting ...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            ESP_LOGI(TAG_w, "Retry to connect (%d/%d)", s_retry_num, ESP_WIFI_MAX_RETRY);
            s_retry_num++;
        }
        else
        {
            ESP_LOGI(TAG_w, "Failed to connect after %d attempts", ESP_WIFI_MAX_RETRY);
            xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_w, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_CONNECTED_BIT);
    }
}

static esp_err_t wifi_init()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_driver_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_driver_cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_network_cfg = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PSW}};

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_network_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_w, "wifi_init finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, ESP_WIFI_CONNECTED_BIT | ESP_WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & ESP_WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_w, "Connected to SSID: %s", ESP_WIFI_SSID);
        return ESP_OK;
    }
    else if (bits & ESP_WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG_w, "Failed to connect to SSID: %s", ESP_WIFI_SSID);
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG_w, "Unexpected event");
        return ESP_FAIL;
    }
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = wifi_init();
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG_w, "WiFi initialization succesful");
    }
    else
    {
        ESP_LOGE(TAG_w, "WiFi initialization failed");
    }
}