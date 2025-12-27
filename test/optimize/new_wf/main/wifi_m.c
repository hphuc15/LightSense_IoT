#include "wifi_m.h"

static const char *TAG = "[WiFi]";

static void WiFiManager_AP_Event_Handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    WiFiManager_t *wifi_manager = (WiFiManager_t *)arg;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START)
    {
        xEventGroupSetBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_APSTART);
        xEventGroupClearBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STASTART);
        wifi_manager->mode = WIFI_MODE_AP;

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

static void WiFiManager_STA_Event_Handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    WiFiManager_t *wifi_manager = (WiFiManager_t *)arg;
    if (!wifi_manager || wifi_manager->event.group == NULL)
    {
        ESP_LOGW(TAG, "[STA_Event_Handler] - WiFi manager context or event group is NULL");
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_APSTART);
        xEventGroupSetBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STASTART);
        wifi_manager->mode = WIFI_MODE_STA;
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STACONNECTED);
        if (wifi_manager->retry_num < ESP_WIFI_STA_MAXIMUM_RETRY)
        {
            wifi_manager->retry_num++;
            ESP_LOGW(TAG, "[STA Mode] - Attempt to connect to the AP (%d/%d)", wifi_manager->retry_num, ESP_WIFI_STA_MAXIMUM_RETRY);
            esp_wifi_connect();
        }
        else
        {
            wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
            wifi_manager->retry_num = 0;
            xEventGroupSetBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STADISCONNECTED);
            ESP_LOGE(TAG, "[STA Mode] - Failed to connect to the AP after %d times, reason: %d", ESP_WIFI_STA_MAXIMUM_RETRY, event->reason);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STACONNECTED);
        xEventGroupClearBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STADISCONNECTED);

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "[STA Mode] - Connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_manager->retry_num = 0;
    }
}

static void WiFiManager_DHCP_Set_CaptivePortal_URL(WiFiManager_t *wifi_manager)
{
    esp_err_t ret;

    // get a handle to configure DHCP with
    esp_netif_t *netif = wifi_manager->netif_ap;
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
 * @brief Response root HTML page to GET request from client
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
 * @brief Parse JSON POST data and save STA configuration and optional settings.
 *
 * @param wifi_manager Pointer to the WiFiManager_t structure.
 * @param string       JSON string received from POST data.
 *
 * @return ESP_OK on success, ESP_FAIL on error.
 */
static esp_err_t WiFiManager_Parse_JSON(WiFiManager_t *wifi_manager, char *string)
{
    if (!wifi_manager || !string)
        return ESP_FAIL;

    cJSON *root = cJSON_Parse(string);
    if (!root)
    {
        ESP_LOGE(TAG, "[JSON] - parse error before: %s\n", cJSON_GetErrorPtr());
        return ESP_FAIL;
    }

    // Write SSID and Password to STA config
    cJSON *wifi = cJSON_GetObjectItem(root, "wifi");
    if (wifi)
    {
        cJSON *ssid = cJSON_GetObjectItem(wifi, "ssid");
        cJSON *password = cJSON_GetObjectItem(wifi, "password");
        if (ssid && cJSON_IsString(ssid))
        {
            strncpy((char *)wifi_manager->conf.sta.ssid, ssid->valuestring, sizeof(wifi_manager->conf.sta.ssid) - 1);
            wifi_manager->conf.sta.ssid[sizeof(wifi_manager->conf.sta.ssid) - 1] = '\0';
        }
        if (password && cJSON_IsString(password))
        {
            strncpy((char *)wifi_manager->conf.sta.password, password->valuestring, sizeof(wifi_manager->conf.sta.password) - 1);
            wifi_manager->conf.sta.password[sizeof(wifi_manager->conf.sta.password) - 1] = '\0';

            if (strlen(password->valuestring) > 0)
            {
                wifi_manager->conf.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
            }
            else
            {
                wifi_manager->conf.sta.threshold.authmode = WIFI_AUTH_OPEN;
            }
        }
    }

    // Write option JSON to WiFiManager_t
    cJSON *option = cJSON_GetObjectItem(root, "option");
    if (option)
    {
        cJSON *enabled = cJSON_GetObjectItem(option, "enabled");
        if (cJSON_IsBool(enabled))
        {
            if (cJSON_IsTrue(enabled))
            {
                wifi_manager->portal.has_options = cJSON_IsTrue(enabled);
            }
            else
            {
                cJSON *option_copy = cJSON_CreateObject();
                cJSON *item = NULL;
                cJSON_ArrayForEach(item, option)
                {
                    if (item->string && strcmp(item->string, "enabled") != 0)
                    {
                        cJSON_AddItemToObject(option_copy, item->string, cJSON_Duplicate(item, 1));
                    }
                }

                // Convert object to JSON format
                char *option_str = cJSON_PrintUnformatted(option_copy);
                if (option_str)
                {
                    strncpy(wifi_manager->portal.options_json, option_str, sizeof(wifi_manager->portal.options_json) - 1);
                    wifi_manager->portal.options_json[sizeof(wifi_manager->portal.options_json) - 1] = '\0';
                    free(option_str);
                }
                cJSON_Delete(option_copy);
            }
        }
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "[Parse JSON] ssid = %s, password = %s, options json = %s", wifi_manager->conf.sta.ssid, wifi_manager->conf.sta.password, wifi_manager->portal.options_json);
    return ESP_OK;
}

static esp_err_t WiFiManager_Root_Post_Handler(httpd_req_t *req)
{
    WiFiManager_t *wifi_manager = (WiFiManager_t *)req->user_ctx;
    if (wifi_manager == NULL)
    {
        ESP_LOGE(TAG, "[Captive Portal] - Root Post Handler, req->user_ctx is NULL!");
        return ESP_FAIL;
    }
    char content[256];
    // Truncate if content length larger than the buffer
    size_t recv_size = MIN(req->content_len, ESP_WIFI_CAPTIVEPORTAL_JSON_SIZE);

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
    wifi_manager->portal.json_len = ret;

    // Parsing JSON and store STA SSID and password (and option json)
    WiFiManager_Parse_JSON(wifi_manager, content);

    ESP_LOGI(TAG, "[Captive Portal] - JSON body: %s", content);
    const char *resp_str = "Data received";
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    WiFiManager_NVS_WriteSTA(wifi_manager);
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "[Captive Portal] Configuration received");
    xEventGroupSetBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STACONF_START);
    return ESP_OK;
}

static httpd_uri_t root_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = WiFiManager_Root_Post_Handler,
    .user_ctx = NULL};

/**
 * @brief Redirects all requests to the root page
 */
static esp_err_t WiFiManager_HTTP_404_Error_Handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_set_status(req, "302 - Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "[Captive Portal] - Redirecting to root");
    return ESP_OK;
}

static httpd_handle_t WiFiManager_Start_WebServer(WiFiManager_t *wifi_manager)
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

        root_post.user_ctx = wifi_manager;

        httpd_register_uri_handler(server, &root_get);
        httpd_register_uri_handler(server, &root_post);

        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, WiFiManager_HTTP_404_Error_Handler);

        wifi_manager->portal.server = server;
    }
    return server;
}

esp_err_t WiFiManager_NVS_WriteSTA(WiFiManager_t *wifi_manager)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    wifi_sta_config_t *sta = &wifi_manager->conf.sta;

    ret = nvs_open(ESP_WIFI_NVS_STA_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[NVS] - Error (%s) when open NVS handle!", esp_err_to_name(ret));
        return ret;
    }

    // Save the STA configuration
    ESP_LOGI(TAG, "[NVS] - Writing data to NVS...");

    ret = nvs_set_str(nvs_handle, "ssid", (const char *)sta->ssid);
    if (ret != ESP_OK)
    {
        goto write_error;
    }
    ret = nvs_set_str(nvs_handle, "password", (const char *)sta->password);
    if (ret != ESP_OK)
    {
        goto write_error;
    }
    /*
        ret = nvs_set_u8(nvs_handle, "authmode", sta->threshold.authmode);
        if (ret != ESP_OK)
        {
            goto write_error;
        }
    */

    // Commit the change
    ret = nvs_commit(nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[NVS] - Failed to commit: %s", esp_err_to_name(ret));
        return ret;
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "[NVS] - STA config saved successfully");
    return ESP_OK;

write_error:
    ESP_LOGE(TAG, "[NVS] - Error (%s) writing data to NVS", esp_err_to_name(ret));
    nvs_close(nvs_handle);
    return ret;
}

esp_err_t WiFiManager_NVS_ReadSTA(WiFiManager_t *wifi_manager)
{
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ret = nvs_open(ESP_WIFI_NVS_STA_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[NVS] - Error (%s) when opening NVS handle!", esp_err_to_name(ret));
        return ret;
    }

    wifi_sta_config_t *sta = &wifi_manager->conf.sta;

    memset(sta, 0, sizeof(wifi_sta_config_t));

    size_t ssid_len = sizeof(sta->ssid);
    ret = nvs_get_str(nvs_handle, "ssid", (char *)sta->ssid, &ssid_len);
    if (ret != ESP_OK)
    {
        if (ret == ESP_ERR_NVS_NOT_FOUND)
        {
            ESP_LOGW(TAG, "[NVS] - No saved STA config found");
        }
        else
        {
            ESP_LOGE(TAG, "[NVS] - Error reading ssid: %s", esp_err_to_name(ret));
        }
        nvs_close(nvs_handle);
        return ret;
    }

    size_t psw_len = sizeof(sta->password);
    if (psw_len > 0)
    {
        sta->threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    }
    else if (psw_len == 0)
    {
        sta->threshold.authmode = WIFI_AUTH_OPEN;
    }

    ret = nvs_get_str(nvs_handle, "password", (char *)sta->password, &psw_len);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[NVS] - Error reading password: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "[NVS] - STA config loaded: SSID: %s", sta->ssid);
    return ESP_OK;
}

void WiFiManager_WiFi_Init(WiFiManager_t *wifi_manager)
{
    esp_err_t ret;

    if (wifi_manager->event.group == NULL)
    {
        wifi_manager->event.group = xEventGroupCreate();
    }
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create event loop, error: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_netif_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize netif, error: %s", esp_err_to_name(ret));
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
        WiFiManager_WiFi_Stop(wifi_manager);
    }

    // WiFi driver config
    wifi_init_config_t wifi_drv_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_drv_cfg));
}

void WiFiManager_STA_Start(WiFiManager_t *wifi_manager)
{
    if (wifi_manager->netif_sta == NULL)
    {
        wifi_manager->netif_sta = esp_netif_create_default_wifi_sta();
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_START, WiFiManager_STA_Event_Handler, wifi_manager, &wifi_manager->event.sta_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, WiFiManager_STA_Event_Handler, wifi_manager, &wifi_manager->event.sta_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, WiFiManager_STA_Event_Handler, wifi_manager, &wifi_manager->event.ip_handle));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_manager->conf));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));
}

void WiFiManager_AP_Start(WiFiManager_t *wifi_manager)
{
    if (wifi_manager->netif_ap == NULL)
    {
        wifi_manager->netif_ap = esp_netif_create_default_wifi_ap();
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, WiFiManager_AP_Event_Handler, wifi_manager, &wifi_manager->event.ap_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, WiFiManager_AP_Event_Handler, wifi_manager, &wifi_manager->event.ap_handle));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, WIFI_EVENT_AP_START, WiFiManager_AP_Event_Handler, wifi_manager, &wifi_manager->event.ap_handle));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_manager->conf));
    ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(100));
}

void WiFiManager_WiFi_Stop(WiFiManager_t *wifi_manager)
{
    if (!wifi_manager)
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

    if (wifi_manager->event.ap_handle)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_manager->event.ap_handle);
        wifi_manager->event.ap_handle = NULL;
    }
    if (wifi_manager->event.sta_handle)
    {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_manager->event.sta_handle);
        wifi_manager->event.sta_handle = NULL;
    }
    if (wifi_manager->event.ip_handle)
    {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_manager->event.ip_handle);
        wifi_manager->event.ip_handle = NULL;
    }

    xEventGroupClearBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_ALL);

    if (mode == WIFI_MODE_STA)
    {
        if (wifi_manager->netif_sta)
        {
            esp_netif_destroy_default_wifi(wifi_manager->netif_sta);
            wifi_manager->netif_sta = NULL;
        }
    }
    else if (mode == WIFI_MODE_AP)
    {
        if (wifi_manager->netif_ap)
        {
            esp_netif_destroy_default_wifi(wifi_manager->netif_ap);
            wifi_manager->netif_ap = NULL;
        }
    }
    else
    {
        if (wifi_manager->netif_sta)
        {
            esp_netif_destroy_default_wifi(wifi_manager->netif_sta);
            wifi_manager->netif_sta = NULL;
        }
        if (wifi_manager->netif_ap)
        {
            esp_netif_destroy_default_wifi(wifi_manager->netif_ap);
            wifi_manager->netif_ap = NULL;
        }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_LOGI(TAG, "[WiFi Stop] - WiFi stopped successfully");
}

void WiFiManager_WiFi_Deinit(WiFiManager_t *wifi_manager)
{
    esp_err_t ret;
    WiFiManager_WiFi_Stop(wifi_manager);

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

void WiFiManager_STA_ConfigViaAP(WiFiManager_t *wifi_manager)
{
    xEventGroupClearBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STACONF_START);

    WiFiManager_AP_Start(wifi_manager);
    xEventGroupWaitBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_APSTART, pdFALSE, pdFALSE, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(200));

    WiFiManager_DHCP_Set_CaptivePortal_URL(wifi_manager);
    WiFiManager_Start_WebServer(wifi_manager);
    dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
    start_dns_server(&config);

    xEventGroupWaitBits(wifi_manager->event.group, ESP_WIFI_EVENT_BIT_STACONF_START, pdTRUE, pdFALSE, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(200));
    WiFiManager_WiFi_Stop(wifi_manager);

    vTaskDelay(pdMS_TO_TICKS(200));
    WiFiManager_NVS_ReadSTA(wifi_manager); // Chưa tối ưu, phải đọc từ NVS

    ESP_LOGI(TAG, "[STA_ConfigViaAP] Switch to STA Mode");
    WiFiManager_STA_Start(wifi_manager);
}