#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <sys/param.h>
#include "sdkconfig.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "esp_http_server.h"
#include "dns_server.h"
#include "driver/gpio.h"

#include "nvs_flash.h"
#include "nvs.h"

#define ESP_WIFI_AP_SSID "ESP32_Config"
#define ESP_WIFI_AP_PSW "ESP32_Config"
#define ESP_WIFI_AP_MAX_STA_CONN 4
#define ESP_WIFI_MAX_RETRY 4
#define ESP_WIFI_STA_CONNECTED_BIT BIT0
#define ESP_WIFI_STA_FAIL_BIT BIT1
#define ESP_WIFI_AP_ACTIVE_BIT BIT2

extern EventGroupHandle_t s_wifi_event_group;

/**
 * @brief Structure that stores Wi-Fi station (STA) credentials.
 *
 * This structure holds the SSID and password provided by the user
 * for connecting the ESP32 to an existing Wi-Fi network in STA mode.
 */
typedef struct
{
    char ssid[33];     /**< Wi-Fi network SSID (null-terminated string, max 32 characters) */
    char password[64]; /**< Wi-Fi network password (null-terminated string, max 63 characters) */
} sta_info_t;
/**
 * @brief Structure that contains all configuration data received from the captive portal.
 *
 * This structure aggregates user-submitted configuration data, including
 * the STA credentials and the target server IP address used by the ESP32
 * after connecting to Wi-Fi.
 */
typedef struct
{
    sta_info_t sta_info; /**< Wi-Fi credentials (SSID and password) */
    char server_ip[40];  /**< Target server IP address or hostname (null-terminated string) */
} post_data_t;

/**
 * @brief Handle Wi-Fi related events (e.g. STA start, got IP, disconnect, etc.)
 *
 * @param arg Optional user argument passed during registration
 * @param event_base The base ID of the event (e.g. WIFI_EVENT, IP_EVENT)
 * @param event_id The specific event ID (e.g. WIFI_EVENT_STA_START)
 * @param event_data Pointer to event-specific data
 */
extern void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/**
 * @brief Initializes and connects ESP32 to an existing Wi-Fi network in Station mode.
 *
 * The function configures Wi-Fi in STA mode using the provided SSID and password.
 *
 * @param sta_info Pointer to a structure containing Wi-Fi SSID and password.
 */
extern void wifi_manager_init_sta(sta_info_t *sta_info);

/**
 * @brief Initializes ESP32 in SoftAP mode to create a captive portal.
 *
 * This function starts an Access Point and runs an embedded web server
 * allowing users to input Wi-Fi credentials for STA connection.
 */
extern void wifi_manager_init_softap(void);

/**
 * @brief Stops all active Wi-Fi interfaces and frees related resources.
 *
 * This function stops both STA and SoftAP modes if they are active,
 * and cleans up allocated memory or event handlers.
 */
extern void wifi_manager_stop(void);

#ifdef CONFIG_ESP_ENABLE_DHCP_CAPTIVEPORTAL
/**
 * @brief Sets the Captive Portal URL for DHCP option 114.
 *
 * This function configures the DHCP server to include a custom captive portal
 * URL so that clients are redirected automatically after connecting to SoftAP.
 */
extern void dhcp_set_captiveportal_url(void);
#endif // CONFIG_ESP_ENABLE_DHCP_CAPTIVEPORTAL

/**
 * @brief HTTP GET request handler for the captive portal root page.
 *
 * Serves the main HTML page where users can enter Wi-Fi credentials.
 *
 * @param req HTTP request handle.
 * @return ESP_OK on success, ESP_FAIL otherwise.
 */
extern esp_err_t wifi_manager_root_get_handler(httpd_req_t *req);

/**
 * @brief HTTP POST request handler for the captive portal root page.
 *
 * Processes the form submission containing SSID, password, and optional server IP.
 *
 * @param req HTTP request handle.
 * @return ESP_OK on success, ESP_FAIL otherwise.
 */
extern esp_err_t wifi_manager_root_post_handler(httpd_req_t *req);

/**
 * @brief Decodes URL-encoded strings from POST form data.
 *
 * Converts characters like '%20' or '+' back to their original representation.
 *
 * @param buf The string buffer to decode (decoded in place).
 */
extern void url_decode(char *buf);

/**
 * @brief HTTP 404 Error handler that redirects to the root page.
 *
 * Any invalid HTTP request will be redirected to the root HTML page.
 *
 * @param req  HTTP request handle.
 * @param err  HTTP error code.
 * @return ESP_OK on success, ESP_FAIL otherwise.
 */
extern esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);

/**
 * @brief Checks whether SoftAP mode is currently active.
 *
 * @return true if SoftAP is running, false otherwise.
 */
extern bool wifi_manager_is_ap_active(void);

/**
 * @brief Checks whether the station (STA) is connected to a Wi-Fi network.
 *
 * @return true if STA is connected, false otherwise.
 */
extern bool wifi_manager_is_sta_connected(void);

/**
 * @brief Saves the Wi-Fi and server configuration to NVS (Non-Volatile Storage).
 *
 * @param post_data Pointer to structure containing Wi-Fi and server data.
 */
extern void wifi_manager_nvs_save_config(post_data_t *post_data);

/**
 * @brief Loads Wi-Fi and server configuration from NVS.
 *
 * @param post_data Pointer to structure where loaded data will be stored.
 */
extern void wifi_manager_nvs_load_config(post_data_t *post_data);

/**
 * @brief Erases all stored Wi-Fi and server configurations from NVS.
 */
extern void wifi_manager_nvs_erase_config(void);

/**
 * @brief Starts the HTTP web server for the Wi-Fi Manager.
 *
 * This function initializes and starts an HTTP server instance that handles
 * GET and POST requests for the captive portal and Wi-Fi configuration pages.
 *
 * @return
 *  - A valid `httpd_handle_t` if the server was started successfully.
 *  - `NULL` if the server failed to start.
 *
 * @note The server typically runs on port 80 and serves the embedded HTML content.
 * @warning Must be called after network interface (SoftAP or STA) initialization.
 */
extern httpd_handle_t wifi_manager_start_webserver(void);

#endif