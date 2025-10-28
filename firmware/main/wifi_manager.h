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

#define ESP_WIFI_AP_SSID CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_AP_PSW CONFIG_ESP_WIFI_PASSWORD
#define ESP_WIFI_AP_MAX_STA_CONN CONFIG_ESP_MAX_STA_CONN
#define ESP_WIFI_STA_SSID "SSIoT-02"
#define ESP_WIFI_STA_PSW "SSIoT-02"
#define ESP_WIFI_MAX_RETRY 4

#define ESP_WIFI_STA_CONNECTED_BIT BIT0
#define ESP_WIFI_STA_FAIL_BIT BIT1
#define ESP_WIFI_AP_ACTIVE_BIT BIT2

extern int s_wifi_retry_num;
extern EventGroupHandle_t s_wifi_event_group;
extern esp_netif_t *netif_sta;
extern esp_netif_t *netif_ap;

// Struct chứa thông tin SSID và PASSWORD của STA cấu hình thông qua captive portal và IP của Flask Server
typedef struct
{
    char ssid[32];
    char password[32];
} sta_info_t;

// Lưu IP của Server chính khi được nhận từ captive portal
extern char main_server_ip[40];

// ============================================== Functions ==========================================================
extern void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
extern void wifi_manager_init_sta(sta_info_t *sta_info);
extern void wifi_manager_init_softap(void);
extern void wifi_manager_stop(void);
#ifdef CONFIG_ESP_ENABLE_DHCP_CAPTIVEPORTAL
extern void dhcp_set_captiveportal_url(void);
#endif // CONFIG_ESP_ENABLE_DHCP_CAPTIVEPORTAL

/**
 * @brief HTTP GET Handler
 */
extern esp_err_t wifi_manager_root_get_handler(httpd_req_t *req);
/**
 * @brief HTTP POST Handler
 */
extern esp_err_t wifi_manager_root_post_handler(httpd_req_t *req);
/**
 * @brief POST data decode function
 * @param buf the data string that need to convert
 */
extern void url_decode(char *buf);

/**
 * @brief HTTP Error (404) Handler - Redirects all requests to the root page
 */
extern esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);

extern bool wifi_manager_is_ap_active(void);
extern bool wifi_manager_is_sta_connected(void);

extern void wifi_manager_nvs_save_config(sta_info_t *network_info, char *main_server_ip);
extern void wifi_manager_nvs_load_config(sta_info_t *network_info, char *main_server_ip);
extern void wifi_manager_nvs_erase_config(void);

extern httpd_handle_t wifi_manager_start_webserver(void);

#endif