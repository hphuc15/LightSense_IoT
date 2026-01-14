#include "WiFiManager.h"

static const char *TAG = "[WM]";
extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");

/**
 * @brief Handle WiFi AP mode events.
 */
static void WiFiManager_AP_Event_Handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    WiFiManager_t *wm = (WiFiManager_t *)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START)
    {
        xEventGroupSetBits(wm->event.group, ESP_WIFI_EVENT_BIT_APSTART);
        xEventGroupClearBits(wm->event.group, ESP_WIFI_EVENT_BIT_STASTART);
        wm->mode = WIFI_MODE_AP;

        esp_netif_ip_info_t ip_info;
        esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
        char ip_addr[16];
        inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);

        ESP_LOGI(TAG, "[AP Mode] - SoftAP started, ssid: %s, password: %s, ip: %s, channel: %d",
                 ESP_WIFI_AP_SSID,
                 ESP_WIFI_AP_PSW,
                 ip_addr,
                 ESP_WIFI_AP_CHANNEL);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "[AP Mode] - Station " MACSTR " join, AID = %d",
                 MAC2STR(event->mac),
                 event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "[AP Mode] - Station " MACSTR " leave, AID = %d, reason = %d",
                 MAC2STR(event->mac),
                 event->aid,
                 event->reason);
    }
}

/**
 * @brief Handle WiFi STA mode and IP events.
 */
static void WiFiManager_STA_Event_Handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    WiFiManager_t *wm = (WiFiManager_t *)arg;
    if (!wm || wm->event.group == NULL)
    {
        ESP_LOGW(TAG, "[STA_Event_Handler] - WiFi manager context or event group is NULL");
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        xEventGroupClearBits(wm->event.group, ESP_WIFI_EVENT_BIT_APSTART);
        xEventGroupSetBits(wm->event.group, ESP_WIFI_EVENT_BIT_STASTART);
        wm->mode = WIFI_MODE_STA;
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(wm->event.group, ESP_WIFI_EVENT_BIT_STACONNECTED);
        if (wm->retry_num < ESP_WIFI_STA_MAXIMUM_RETRY)
        {
            wm->retry_num++;
            ESP_LOGW(TAG, "[STA Mode] - Attempt to connect to the AP (%d/%d)", wm->retry_num, ESP_WIFI_STA_MAXIMUM_RETRY);
            esp_wifi_connect();
        }
        else
        {
            wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
            wm->retry_num = 0;
            xEventGroupSetBits(wm->event.group, ESP_WIFI_EVENT_BIT_STADISCONNECTED);
            ESP_LOGE(TAG, "[STA Mode] - Failed to connect to the AP after %d times, reason: %d", ESP_WIFI_STA_MAXIMUM_RETRY, event->reason);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wm->event.group, ESP_WIFI_EVENT_BIT_STACONNECTED);
        xEventGroupClearBits(wm->event.group, ESP_WIFI_EVENT_BIT_STADISCONNECTED);

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "[STA Mode] Connected to AP %s, IP: " IPSTR, (char *)wm->config.sta.ssid, IP2STR(&event->ip_info.ip));
        wm->retry_num = 0;
    }
}

/**
 * @brief Configure DHCP captive portal redirect URL.
 */
static void WiFiManager_DHCP_Set_CaptivePortal_URL(WiFiManager_t *wm)
{
    esp_err_t ret;

    // get a handle to configure DHCP with
    esp_netif_t *netif = wm->netif_ap;
    if (!netif)
    {
        ESP_LOGE(TAG, "[DHCP] netif_ap is null");
        return;
    }

    // get the IP of the access point to redirect to
    esp_netif_ip_info_t ip_info;
    ret = esp_netif_get_ip_info(netif, &ip_info); // Get AP IP
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[DHCP] Failed to get IP info, error: %s", esp_err_to_name(ret));
        return;
    }

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16); // Convert IPv4 (uint32_t) to string ("a.b.c.d") format
    ESP_LOGI(TAG, "[AP Mode] - Set up softAP with IP: %s", ip_addr);

    // turn the IP into a URI
    char *captiveportal_uri = (char *)malloc(32 * sizeof(char));
    assert(captiveportal_uri && "Failed to allocate captiveportal_uri");
    strcpy(captiveportal_uri, "http://");
    strcat(captiveportal_uri, ip_addr);

    // set the DHCP option 114
    ret = esp_netif_dhcps_stop(netif);
    if (ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED)
    {
        ESP_LOGW(TAG, "DHCP stop warning: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, captiveportal_uri, strlen(captiveportal_uri));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[DHCP] - Failed to set DHCP option: %s", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "[DHCP] Captive Portal URI set: %s", &captiveportal_uri);
    }

    ret = esp_netif_dhcps_start(netif);
    if (ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)
    {
        ESP_LOGW(TAG, "Failed to start DHCP: %s", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(TAG, "[DHCP] Server started succesfully");
    }
}

/**
 * @brief HTTP GET handler serving captive portal root page.
 */
static esp_err_t WiFiManager_Root_Get_Handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start; // Length of raw HTML

    ESP_LOGI(TAG, "[Captive Portal] - Root Server");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}
static httpd_uri_t root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = WiFiManager_Root_Get_Handler};

/**
 * @brief HTTP POST handler receiving captive portal configuration.
 */
static esp_err_t WiFiManager_Root_Post_Handler(httpd_req_t *req)
{
    WiFiManager_t *wm = (WiFiManager_t *)req->user_ctx;
    if (wm == NULL)
    {
        ESP_LOGE(TAG, "[Captive Portal] - Root Post Handler, req->user_ctx is NULL!");
        return ESP_FAIL;
    }
    char content[256];
    // Truncate if content length larger than the buffer
    size_t recv_size;
    if (req->content_len < ESP_WIFI_CAPTIVEPORTAL_JSON_SIZE)
    {
        recv_size = req->content_len;
    }
    else
    {
        recv_size = ESP_WIFI_CAPTIVEPORTAL_JSON_SIZE;
    }

    int ret = httpd_req_recv(req, content, recv_size); // Read HTTP content data (body) from the HTTP request into provided buffer
    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }

        return ESP_FAIL;
    }
    content[ret] = '\0';
    wm->portal.json_len = ret;

    strncpy(wm->portal.json, content, PORTAL_JSON_MAX_LEN - 1);
    wm->portal.json[PORTAL_JSON_MAX_LEN - 1] = '\0';
    wm->portal.json_len = strlen(wm->portal.json);

    char *ssid = (char *)wm->config.sta.ssid;
    char *password = (char *)wm->config.sta.password;

    WiFiManager_JSON_GetValue(wm->portal.json, "wifi", "ssid", ssid, sizeof(wm->config.sta.ssid));
    WiFiManager_JSON_GetValue(wm->portal.json, "wifi", "password", password, sizeof(wm->config.sta.password));
    if (!strlen((char *)wm->config.sta.password))
    {
        wm->config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    else
    {
        wm->config.sta.threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
    }
    if (WiFiManager_GetBoolJSON(wm->portal.json, "option", "enabled", false))
    {
        wm->portal.option_flag = true;
    }

    ESP_LOGI(TAG, "[Captive Portal] - JSON body: %s", content);
    const char *resp_str = "Data received";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    WiFiManager_NVS_SetSTA(wm);
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "[Captive Portal] Configuration received");
    xEventGroupSetBits(wm->event.group, ESP_WIFI_EVENT_BIT_STACONF_START);
    return ESP_OK;
}
static httpd_uri_t root_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = WiFiManager_Root_Post_Handler,
    .user_ctx = NULL};

/**
 * @brief Redirect all unknown HTTP requests to root page.
 */
static esp_err_t WiFiManager_HTTP_404_Error_Handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_set_status(req, "302 - Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "[Captive Portal] - Redirecting to root");
    return ESP_OK;
}

/**
 * @brief Start captive portal web server.
 * @return HTTP server handle.
 */
static httpd_handle_t WiFiManager_Start_WebServer(WiFiManager_t *wm)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 6;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 180; // Timeout for waiting client data: 3 minutes

    // Start the httpd server
    ESP_LOGI(TAG, "[Captive Portal] - Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "[Captive Portal] - Registering URI handlers");

        root_post.user_ctx = wm;

        httpd_register_uri_handler(server, &root_get);
        httpd_register_uri_handler(server, &root_post);

        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, WiFiManager_HTTP_404_Error_Handler);

        wm->portal.server = server;
    }
    return server;
}

// ===================================================================================================
void WiFiManager_JSON_GetValue(const char *json, const char *parent, const char *key, char *out, size_t out_len)
{
    if (!json || !parent || !key || !out || out_len == 0)
    {
        if (out)
            out[0] = '\0';
        return;
    }

    cJSON *root = cJSON_Parse(json);
    if (!root)
    {
        out[0] = '\0';
        return;
    }

    cJSON *obj = cJSON_GetObjectItemCaseSensitive(root, parent);
    if (!cJSON_IsObject(obj))
    {
        out[0] = '\0';
        cJSON_Delete(root);
        return;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!cJSON_IsString(item) || !item->valuestring)
    {
        out[0] = '\0';
        cJSON_Delete(root);
        return;
    }

    strncpy(out, item->valuestring, out_len - 1);
    out[out_len - 1] = '\0';

    cJSON_Delete(root);
}

bool WiFiManager_GetBoolJSON(const char *json, const char *parent, const char *key, bool default_val)
{
    cJSON *root = cJSON_Parse(json);
    if (!root)
    {
        return default_val;
    }
    cJSON *obj = cJSON_GetObjectItemCaseSensitive(root, parent);
    if (!cJSON_IsObject(obj))
    {
        cJSON_Delete(root);
        return default_val;
    }

    cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    bool val = cJSON_IsTrue(item);

    cJSON_Delete(root);
    return val;
}

void WiFiManager_NVS_SetValue(const char *namespace, const char *key, const char *value)
{
    esp_err_t err;
    nvs_handle_t handle;

    err = nvs_open(namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "[NVS_SetValue] Error (%s) when open NVS handle.", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "[NVS_SetValue] Writing data to NVS...");

    err = nvs_set_str(handle, key, value);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "[NVS_SetValue] Error (%s) writing data to NVS", esp_err_to_name(err));
        nvs_close(handle);
        return;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "[NVS_SetValue] Failed to commit: %s", esp_err_to_name(err));
        nvs_close(handle);
        return;
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "[NVS_SetValue] %s written to NVS", key);
}
void WiFiManager_NVS_GetValue(const char *namespace, const char *key, char *out, size_t out_len)
{
    esp_err_t err;
    nvs_handle_t handle;

    err = nvs_open(namespace, NVS_READWRITE, &handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "[NVS_GetValue] Error (%s) when open NVS handle.", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "[NVS_GetValue] Getting data from NVS...");

    err = nvs_get_str(handle, key, out, &out_len);
    if (err != ESP_OK)
    {
        if (err == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGE(TAG, "[NVS_GetValue] The key %s doesn't exist", key);
        }
        else
        {
            ESP_LOGE(TAG, "[NVS_GetValue] Error (%s) reading data from NVS", esp_err_to_name(err));
        }
        nvs_close(handle);
        return;
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "[NVS_GetValue] %s read from NVS", key);
}

void WiFiManager_NVS_SetSTA(WiFiManager_t *wm)
{
    char *ssid = (char *)wm->config.sta.ssid;
    char *password = (char *)wm->config.sta.password;
    WiFiManager_NVS_SetValue(ESP_WIFI_NVS_STA_NAMESPACE, "ssid", ssid);
    WiFiManager_NVS_SetValue(ESP_WIFI_NVS_STA_NAMESPACE, "password", password);

    ESP_LOGI(TAG, "[NVS_SetSTA] STA config written to NVS");
}
void WiFiManager_NVS_GetSTA(WiFiManager_t *wm)
{
    memset(&wm->config.sta, 0, sizeof(wm->config.sta));
    char *ssid = (char *)wm->config.sta.ssid;
    char *password = (char *)wm->config.sta.password;
    WiFiManager_NVS_GetValue(ESP_WIFI_NVS_STA_NAMESPACE, "ssid", ssid, sizeof(wm->config.sta.ssid));
    WiFiManager_NVS_GetValue(ESP_WIFI_NVS_STA_NAMESPACE, "password", password, sizeof(wm->config.sta.password));

    size_t psw_len = strlen(password);
    if (psw_len > 0)
    {
        wm->config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    else if (psw_len == 0)
    {
        wm->config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    ESP_LOGI(TAG, "[NVS_GetSTA] STA config read from NVS");
}

void WiFiManager_Init(WiFiManager_t *wm)
{
    esp_err_t ret;

    if (wm->event.group == NULL)
    {
        wm->event.group = xEventGroupCreate();
    }
    ret = esp_event_loop_create_default();
    if (ret == ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(TAG, "Default event loop has already been created");
    }
    else if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create event loop, error: %s", esp_err_to_name(ret));
        return;
    }

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_mode_t mode;
    ret = esp_wifi_get_mode(&mode);
    if (ret != ESP_ERR_WIFI_NOT_INIT)
    {
        WiFiManager_WiFi_Stop(wm);
    }

    // WiFi driver config
    wifi_init_config_t wifi_drv_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_drv_cfg));
}

void WiFiManager_STA_Start(WiFiManager_t *wm)
{
    if (wm->netif_sta == NULL)
    {
        wm->netif_sta = esp_netif_create_default_wifi_sta();
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_START, WiFiManager_STA_Event_Handler, wm, &wm->event.sta_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, WiFiManager_STA_Event_Handler, wm, &wm->event.sta_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, WiFiManager_STA_Event_Handler, wm, &wm->event.ip_handle));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wm->config));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));
}

void WiFiManager_AP_Start(WiFiManager_t *wm)
{
    if (wm->netif_ap == NULL)
    {
        wm->netif_ap = esp_netif_create_default_wifi_ap();
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, WiFiManager_AP_Event_Handler, wm, &wm->event.ap_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, WiFiManager_AP_Event_Handler, wm, &wm->event.ap_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_START, WiFiManager_AP_Event_Handler, wm, &wm->event.ap_handle));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wm->config));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));
}

void WiFiManager_WiFi_Stop(WiFiManager_t *wm)
{
    if (!wm)
    {
        ESP_LOGE(TAG, "[WiFi Stop] - WiFi manager is NULL");
        return;
    }

    esp_err_t ret;
    wifi_mode_t mode;

    ret = esp_wifi_get_mode(&mode);
    if (ret != ESP_OK)
    {
        ESP_LOGW(TAG, "[WiFi Stop] - Failed to get WiFi mode: %s", esp_err_to_name(ret));
        mode = WIFI_MODE_NULL;
    }

    ESP_LOGI(TAG, "[WiFi Stop] - Stopping WiFi (mode: %d)", mode);

    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA)
    {
        ret = esp_wifi_disconnect();
        if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_STARTED)
        {
            ESP_LOGW(TAG, "[WiFi Stop] - Disconnect failed: %s", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGI(TAG, "[WiFi Stop] - STA disconnected");
        }
    }

    ret = esp_wifi_stop();
    if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_STARTED)
    {
        ESP_LOGE(TAG, "[WiFi Stop] - WiFi stop failed: %s", esp_err_to_name(ret));
        return;
    }

    if (wm->event.ap_handle)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wm->event.ap_handle);
        wm->event.ap_handle = NULL;
    }
    if (wm->event.sta_handle)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wm->event.sta_handle);
        wm->event.sta_handle = NULL;
    }
    if (wm->event.ip_handle)
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wm->event.ip_handle);
        wm->event.ip_handle = NULL;
    }

    xEventGroupClearBits(wm->event.group, ESP_WIFI_EVENT_BIT_ALL);

    if (mode == WIFI_MODE_STA)
    {
        if (wm->netif_sta)
        {
            esp_netif_destroy_default_wifi(wm->netif_sta);
            wm->netif_sta = NULL;
        }
    }
    else if (mode == WIFI_MODE_AP)
    {
        if (wm->netif_ap)
        {
            esp_netif_destroy_default_wifi(wm->netif_ap);
            wm->netif_ap = NULL;
        }
    }
    else
    {
        if (wm->netif_sta)
        {
            esp_netif_destroy_default_wifi(wm->netif_sta);
            wm->netif_sta = NULL;
        }
        if (wm->netif_ap)
        {
            esp_netif_destroy_default_wifi(wm->netif_ap);
            wm->netif_ap = NULL;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "[WiFi Stop] - WiFi stopped successfully");
}

void WiFiManager_WiFi_Deinit(WiFiManager_t *wm)
{
    esp_err_t ret;
    WiFiManager_WiFi_Stop(wm);

    ret = esp_wifi_deinit();
    if (ret == ESP_ERR_WIFI_NOT_INIT)
    {
        ESP_LOGE(TAG, "[WiFi Deinit] WiFi is not initialized by esp_wifi_init");
        return;
    }

    ret = nvs_flash_deinit();
    if (ret == ESP_ERR_NVS_NOT_INITIALIZED)
    {
        ESP_LOGE(TAG, "[WiFi Deinit] The storage was not initialized prior to this call");
        return;
    }

    ret = esp_event_loop_delete_default();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[WiFi Deinit] error: %s", esp_err_to_name(ret));
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "[WiFi Deinit] - WiFi deinitialized");
}

void WiFiManager_STA_ConfigViaAP(WiFiManager_t *wm)
{
    xEventGroupClearBits(wm->event.group, ESP_WIFI_EVENT_BIT_STACONF_START);

    WiFiManager_AP_Start(wm);
    xEventGroupWaitBits(wm->event.group, ESP_WIFI_EVENT_BIT_APSTART, pdFALSE, pdFALSE, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(200));

    WiFiManager_DHCP_Set_CaptivePortal_URL(wm);
    WiFiManager_Start_WebServer(wm);
    dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
    start_dns_server(&config);

    xEventGroupWaitBits(wm->event.group, ESP_WIFI_EVENT_BIT_STACONF_START, pdTRUE, pdFALSE, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(200));
    WiFiManager_WiFi_Stop(wm);

    vTaskDelay(pdMS_TO_TICKS(200));
    WiFiManager_NVS_GetSTA(wm);

    ESP_LOGI(TAG, "[STA_ConfigViaAP] Switch to STA Mode");
    WiFiManager_STA_Start(wm);
}

void WiFiManager_CaptivePortal_GetOption(WiFiManager_t *wm, const char *key, char *out, size_t out_len)
{
    WiFiManager_JSON_GetValue(wm->portal.json, "option", key, out, out_len);
    return;
}

bool WiFiManager_IsStaActive(WiFiManager_t *wm){
    EventBits_t bits = xEventGroupWaitBits(wm->event.group, ESP_WIFI_EVENT_BIT_STASTART, pdFALSE, pdTRUE, 0);
    return (bits & ESP_WIFI_EVENT_BIT_STASTART) != 0;
}

bool WiFiManager_IsApActive(WiFiManager_t *wm){
    EventBits_t bits = xEventGroupWaitBits(wm->event.group, ESP_WIFI_EVENT_BIT_APSTART, pdFALSE, pdTRUE, 0);
    return (bits & ESP_WIFI_EVENT_BIT_APSTART) != 0;
}