// Chưa thêm AP
// Trong C:\Users\hphuc\Desktop\LightSense_IoT\test\AfterCompletedProject\esp-sta-ap-button-module đã có AP+STA

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "sdkconfig.h"
#include "esp_http_client.h"

#define ESP_WIFI_STA_SSID "SSIoT-02"
#define ESP_WIFI_STA_PSW "SSIoT-02"
#define ESP_WIFI_AP_SSID "MyESP32"
#define ESP_WIFI_AP_PSW "MyESP32"
#define ESP_WIFI_MAX_RETRY 3
#define ESP_WIFI_FAIL_BIT BIT0
#define ESP_WIFI_CONNECTED_BIT BIT1

static const char *TAG_w = "wifi";
static const char *TAG_h = "http";

EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

// WiFi
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        ESP_LOGI(TAG_w, "WiFi Started");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_w, "Retry to connect to AP (%d/%d)", s_retry_num, ESP_WIFI_MAX_RETRY);
        }
        else
        {
            ESP_LOGI(TAG_w, "Failed to connect to the AP");
            xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_FAIL_BIT);
        }
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_w, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_FAIL_BIT);
        s_retry_num = 0;
    }
}
static esp_err_t sta_init()
{
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_driver_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_driver_cfg));

    esp_event_handler_instance_t sta_instance;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &sta_instance);

    wifi_config_t wifi_network_cfg = {
        .sta = {
            .ssid = ESP_WIFI_STA_SSID,
            .password = ESP_WIFI_STA_PSW}};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_network_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG_w, "sta_init finished");

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
    return ESP_OK;
}

// HTTP
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (&evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG_h, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG_h, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT: // Client đã gửi http header thành công, không chứa key: value, chỉ thông báo
        ESP_LOGI(TAG_h, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER: // Nhận header từ server, có chứa key: value
        ESP_LOGI(TAG_h, "HTTP_EVENT_ON_HEADER, key: %s, value: %s", (&evt->header_key), (&evt->header_value));
        break;
    case HTTP_EVENT_ON_DATA: // Nhận data body từ server
        ESP_LOGI(TAG_h, "HTTP_EVENT_ON_DATA, len: %d", &evt->data);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            printf("%.*s\n", (evt->data_len), (char *)evt->data);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG_h, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_h, "HTTP_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
    return ESP_OK;
}
static void http_get_init(){
    esp_http_client_config_t config = {
        .
    }
}

// MAIN
void app_main()
{
    s_wifi_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NOT_ENOUGH_SPACE || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    sta_init();
}