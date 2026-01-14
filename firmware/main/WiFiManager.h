#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include "stdbool.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "lwip/inet.h"
#include "esp_http_server.h"
#include "dns_server.h"

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

#define PORTAL_JSON_MAX_LEN 512

typedef wifi_config_t WiFiManager_Config_t;

typedef struct WiFiManager_Event_t
{
    EventGroupHandle_t group;
    esp_event_handler_instance_t ap_handle;
    esp_event_handler_instance_t sta_handle;
    esp_event_handler_instance_t ip_handle;
} WiFiManager_Event_t;

typedef struct CaptivePortal_t
{
    httpd_handle_t server;
    size_t json_len;
    char json[PORTAL_JSON_MAX_LEN]; // Full json
    bool option_flag;
} WiFiManager_Portal_t;

typedef struct WiFiManager_t
{
    // Config
    WiFiManager_Config_t config;
    // Event
    WiFiManager_Event_t event;
    // Network Interface
    esp_netif_t *netif_sta;
    esp_netif_t *netif_ap;
    // State
    int retry_num;
    wifi_mode_t mode;
    // Config Via AP
    WiFiManager_Portal_t portal;
} WiFiManager_t;

/**
 * @brief      Get a string value from a JSON object inside a parent object.
 *
 * @param[in]  json      The JSON string
 * @param[in]  parent    The parent key under which the target key exists
 * @param[in]  key       The key whose value to retrieve
 * @param[out] out       Buffer to store the retrieved string
 * @param[in]  out_len   Size of the output buffer
 *
 *
 * @example
 * const char *json = "{\"wifi\":{\"ssid\":\"B9 106\",\"password\":\"B91062005@\"},"
 *                    "\"option\":{\"enabled\":true,\"server_ip\":\"192.168.1.7\",\"server_port\":\"5000\"}}";
 * char ssid[32];
 * WiFiManager_JSON_GetValue(json, "wifi", "ssid", ssid, sizeof(ssid));
 * // ssid will contain "B9 106"
 */
void WiFiManager_JSON_GetValue(const char *json, const char *parent, const char *key, char *out, size_t out_len);

bool WiFiManager_GetBoolJSON(const char *json, const char *parent, const char *key, bool default_val);

/**
 * @brief Store a string into NVS.
 * @param namespace  NVS namespace.
 * @param key        Key name.
 * @param value      String value to store.
 */
void WiFiManager_NVS_SetValue(const char *namespace, const char *key, const char *value);

/**
 * @brief Read a string from NVS.
 * @param namespace  NVS namespace.
 * @param key        Key name.
 * @param out        Output buffer.
 * @param out_len    Buffer length.
 */
void WiFiManager_NVS_GetValue(const char *namespace, const char *key, char *out, size_t out_len);

/**
 * @brief Save STA WiFi credentials to NVS.
 * @param wm  WiFi manager handle.
 */
void WiFiManager_NVS_SetSTA(WiFiManager_t *wm);

/**
 * @brief Load STA WiFi credentials from NVS.
 * @param wm  WiFi manager handle.
 */
void WiFiManager_NVS_GetSTA(WiFiManager_t *wm);

/**
 * @brief Get a uint8 value from a Captive Portal option in JSON.
 *
 * @param[in] json JSON string containing Captive Portal options
 * @param[in] key Key under "option" to retrieve
 * @param[out] out Buffer to store the value
 * @param[in] out_len Size of the output buffer
 */
void WiFiManager_CaptivePortal_GetOption(WiFiManager_t *wm, const char *key, char *out, size_t out_len);

void WiFiManager_Init(WiFiManager_t *wm);
void WiFiManager_STA_Start(WiFiManager_t *wm);
void WiFiManager_AP_Start(WiFiManager_t *wm);
void WiFiManager_WiFi_Stop(WiFiManager_t *wm);
void WiFiManager_WiFi_Deinit(WiFiManager_t *wm);
void WiFiManager_STA_ConfigViaAP(WiFiManager_t *wm);

bool WiFiManager_IsStaActive(WiFiManager_t *wm);
bool WiFiManager_IsApActive(WiFiManager_t *wm);

#endif