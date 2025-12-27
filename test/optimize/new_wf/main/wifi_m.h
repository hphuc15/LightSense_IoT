#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_http_server.h"
#include "dns_server.h"
#include "lwip/inet.h"
#include "sys/param.h"

#include "cJSON.h"

#define ESP_WIFI_AP_SSID "ESP32_Config"
#define ESP_WIFI_AP_PSW "ESP32_Config"
#define ESP_WIFI_AP_MAX_STA_CONN 1
#define ESP_WIFI_AP_CHANNEL 6
#define ESP_WIFI_AP_GTK_REKEY_INTERVAL 600
#define ESP_WIFI_CAPTIVEPORTAL_JSON_SIZE 256

#define ESP_WIFI_STA_MAXIMUM_RETRY 3
#define ESP_WIFI_NVS_STA_NAMESPACE "sta"

#define ESP_WIFI_EVENT_BIT_STASTART BIT0
#define ESP_WIFI_EVENT_BIT_STADISCONNECTED BIT1
#define ESP_WIFI_EVENT_BIT_STACONNECTED BIT2
#define ESP_WIFI_EVENT_BIT_APSTART BIT3
#define ESP_WIFI_EVENT_BIT_STACONF_START BIT4
#define ESP_WIFI_EVENT_BIT_ALL (         \
    ESP_WIFI_EVENT_BIT_STASTART |        \
    ESP_WIFI_EVENT_BIT_STADISCONNECTED | \
    ESP_WIFI_EVENT_BIT_STACONNECTED |    \
    ESP_WIFI_EVENT_BIT_APSTART)

#define ESP_WIFI_AP_CONFIG_DEFAULT()                                                    \
    (wifi_ap_config_t)                                                                  \
    {                                                                                   \
        .ssid = ESP_WIFI_AP_SSID,                                                       \
        .ssid_len = strlen(ESP_WIFI_AP_SSID),                                           \
        .password = ESP_WIFI_AP_PSW,                                                    \
        .max_connection = ESP_WIFI_AP_MAX_STA_CONN,                                     \
        .authmode = (strlen(ESP_WIFI_AP_PSW)) ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN \
    }

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");

typedef struct CaptivePortal_t
{
    httpd_handle_t server;
    size_t json_len;
    bool has_options;       // Set to true when sending additional data other than STA configuration
    char options_json[160]; // Variable to store data that sending other than STA configuration
} CaptivePortal_t;

typedef struct WiFiEvent_t
{
    EventGroupHandle_t group;                // Event group used to synchronize WiFi states
    esp_event_handler_instance_t ap_handle;  // Registered event handler instance for WiFi AP events
    esp_event_handler_instance_t sta_handle; // Registered event handler instance for WiFi STA events
    esp_event_handler_instance_t ip_handle;  // Registered event handler instance for IP events (STA_GOT_IP)
} WiFiEvent_t;

typedef struct WiFiManager_t
{
    wifi_mode_t mode;       // Current WiFi operating mode (WIFI_MODE_STA, WIFI_MODE_AP, ...)
    wifi_config_t conf;     // WiFi configuration (SSID, password, channel, authentication mode, ...)
    int retry_num;          // Number of retry attempts when STA connection is lost
    WiFiEvent_t event;      // WiFi-related event group and event handler instances
    esp_netif_t *netif_sta; // Network interface handle for STA mode
    esp_netif_t *netif_ap;  // Network interface handle for AP mode
    CaptivePortal_t portal; // Captive portal configuration and runtime data
} WiFiManager_t;

/**
 * @brief Save STA configuration from wifi_manager to NVS.
 *
 * @param [in]*wifimanager WiFi Manager handle
 */
esp_err_t WiFiManager_NVS_WriteSTA(WiFiManager_t *wifi_manager);

/**
 * @brief Read STA configuration from NVS to wifi_manager.
 *
 * @param [in]*wifi_manager WiFi Manager handle
 */
esp_err_t WiFiManager_NVS_ReadSTA(WiFiManager_t *wifi_manager);

/**
 * @brief Initialize the WiFi stack and related components for the WiFi Manager.
 *
 * @param wifi_manager Pointer to the WiFiManager_t structure that manages all WiFi states.
 */
void WiFiManager_WiFi_Init(WiFiManager_t *wifi_manager);

/**
 * @brief Start WiFi in Station (STA) mode.
 *
 * @param wifi_manager Pointer to the WiFiManager_t structure.
 */
void WiFiManager_STA_Start(WiFiManager_t *wifi_manager);

/**
 * @brief Start WiFi in Access Point (AP) mode.
 *
 * @param wifi_manager Pointer to the WiFiManager_t structure.
 */
void WiFiManager_AP_Start(WiFiManager_t *wifi_manager);

/**
 * @brief Stop WiFi and release related resources.
 *
 * @param wifi_manager Pointer to the WiFiManager_t structure.
 */
void WiFiManager_WiFi_Stop(WiFiManager_t *wifi_manager);

/**
 * @brief Configure WiFi Station settings via Access Point mode
 *
 * @param wifi_manager Pointer to the WiFiManager_t structure.
 */
void WiFiManager_STA_ConfigViaAP(WiFiManager_t *wifi_manager);
