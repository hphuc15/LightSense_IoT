#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "sdkconfig.h"

#define ESP_WIFI_STA_SSID "SSIoT-02"
#define ESP_WIFI_STA_PSW "SSIoT-02"
#define ESP_WIFI_AP_SSID "ap_ssid"
#define ESP_WIFI_AP_PSW "ap_psw"
#define ESP_WIFI_MAX_RETRY 3
#define ESP_WIFI_FAIL_BIT BIT0
#define ESP_WIFI_CONNECTED_BIT BIT1

static const char *TAG = "wifi";
static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi started");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (%d/%d)", s_retry_num, ESP_WIFI_MAX_RETRY);
        }
        else
        {
            ESP_LOGI(TAG, "Failed to connect to the AP");
            xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_FAIL_BIT);
            vTaskDelay(pdMS_TO_TICKS(50));
            ESP_LOGI(TAG, "Starting AP, SSID: %s, Password: %s", ESP_WIFI_AP_SSID, ESP_WIFI_AP_PSW);
        }
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_CONNECTED_BIT);
        s_retry_num = 0;
    }
    else if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " join, AID: %d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "Station " MACSTR " out, AID: %d, reason: %d", MAC2STR(event->mac), event->reason);
    }
}

static esp_err_t sta_init()
{
    esp_netif_create_default_wifi_sta();

    esp_event_handler_instance_t sta_instance;
    esp_event_handler_instance_t ip_instance;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &sta_instance);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &ip_instance);

    wifi_config_t wifi_network_cfg = {
        .sta = {
            .ssid = ESP_WIFI_STA_SSID,
            .password = ESP_WIFI_STA_PSW}};

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_network_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

static esp_err_t ap_init()
{
    esp_netif_create_default_wifi_ap();

    esp_event_handler_instance_t wifi_instance;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &wifi_instance);

    wifi_config_t wifi_network_cfg = {
        .ap = {
            .ssid = ESP_WIFI_AP_SSID,
            .password = ESP_WIFI_AP_PSW,
            .channel = 1,
            .max_connection = 3}};

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_network_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NOT_ENOUGH_SPACE || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t wifi_driver_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_driver_cfg));

    sta_init();
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        ESP_WIFI_FAIL_BIT | ESP_WIFI_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

    if (bits & ESP_WIFI_FAIL_BIT)
    { // bits & ESP_WIFI_FAIL_BIT -> kiểm tra bit FAIL có bật không
        ESP_LOGI(TAG, "STA failed, switching to AP mode");
        ap_init();
    }
    else
    {
        ESP_LOGI(TAG, "STA connected succesfully");
    }
}