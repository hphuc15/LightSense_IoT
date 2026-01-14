// remove server ip info to modular programing (future)
#include "wifi_manager.h"

int s_wifi_retry_num = 0;
EventGroupHandle_t s_wifi_event_group;
const char *TAG = "WiFi";
const char *TAG_NVS = "NVS";

esp_netif_t *netif_sta = NULL;
esp_netif_t *netif_ap = NULL;

extern const char root_start[] asm("_binary_root_html_start");
extern const char root_end[] asm("_binary_root_html_end");

extern post_data_t post_data;

const httpd_uri_t root_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = wifi_manager_root_get_handler};

const httpd_uri_t root_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = wifi_manager_root_post_handler};


void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_AP_ACTIVE_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d", MAC2STR(event->mac), event->aid, event->reason);
        xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_AP_ACTIVE_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupClearBits(s_wifi_event_group, ESP_WIFI_STA_CONNECTED_BIT);
        if (s_wifi_retry_num < ESP_WIFI_MAX_RETRY)
        {
            s_wifi_retry_num++;
            ESP_LOGI(TAG, "Attempt to connect to the AP (%d/%d)", s_wifi_retry_num, ESP_WIFI_MAX_RETRY);
            esp_wifi_connect();
        }
        else
        {
            s_wifi_retry_num = 0;
            ESP_LOGI(TAG, "Fail to connect to the AP");
            xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_STA_FAIL_BIT);
            ESP_LOGI(TAG, "Starting AP mode...");
            wifi_manager_init_softap();
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupClearBits(s_wifi_event_group, ESP_WIFI_STA_FAIL_BIT);
        xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_STA_CONNECTED_BIT);
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_retry_num = 0;
    }
}

bool wifi_manager_is_sta_connected()
{
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    return (bits & ESP_WIFI_STA_CONNECTED_BIT);
}

bool wifi_manager_is_ap_active()
{
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    return (bits & ESP_WIFI_AP_ACTIVE_BIT);
}

void wifi_manager_stop(void)
{
    // esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    // esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);

    if (wifi_manager_is_sta_connected() || wifi_manager_is_ap_active())
    {
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
        if (netif_sta != NULL)
        {
            esp_netif_destroy_default_wifi(netif_sta);
            netif_sta = NULL;
        }
        if (netif_ap != NULL)
        {
            esp_netif_destroy_default_wifi(netif_ap);
            netif_ap = NULL;
        }
    }
    ESP_LOGI("WiFi", "WiFi stopped succesfully");
    vTaskDelay(pdMS_TO_TICKS(10));
}

void wifi_manager_init_sta(sta_info_t *sta_info)
{
    if (wifi_manager_is_ap_active())
    {
        wifi_manager_stop();
    }
    if (netif_sta == NULL)
    {
        netif_sta = esp_netif_create_default_wifi_sta();
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {0};

    wifi_manager_nvs_load_config(&post_data); // Lấy data trong nvs lưu vào biến post_data_t post_data
    if (strlen(sta_info->ssid) == 0 && strlen(sta_info->password) == 0)
    {
        wifi_manager_init_softap();
    }
    // truyền cấu hình ssid và password lấy từ portal vào cấu hình wifi driver
    strncpy((char *)wifi_config.sta.ssid, sta_info->ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, sta_info->password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_manager_init_softap(void)
{
    if (wifi_manager_is_sta_connected())
    {
        wifi_manager_stop();
    }

    // Initialize Wi-Fi including netif with default config
    if (netif_ap == NULL)
    {
        netif_ap = esp_netif_create_default_wifi_ap();
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_AP_SSID,
            .ssid_len = strlen(ESP_WIFI_AP_SSID),
            .password = ESP_WIFI_AP_PSW,
            .max_connection = ESP_WIFI_AP_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK}};
    if (strlen(ESP_WIFI_AP_PSW) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_AP_ACTIVE_BIT);

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_manager_init_softap finished. SSID:'%s' password:'%s'", ESP_WIFI_AP_SSID, ESP_WIFI_AP_PSW);

#ifdef CONFIG_ESP_ENABLE_DHCP_CAPTIVEPORTAL
    dhcp_set_captiveportal_url();
#endif
    // Start the server for the first time
    wifi_manager_start_webserver();
    // Start the DNS server that will redirect all queries to the softAP IP
    dns_server_config_t config = DNS_SERVER_CONFIG_SINGLE("*" /* all A queries */, "WIFI_AP_DEF" /* softAP netif ID */);
    start_dns_server(&config);
}

#ifdef CONFIG_ESP_ENABLE_DHCP_CAPTIVEPORTAL
void dhcp_set_captiveportal_url(void)
{
    // get the IP of the access point to redirect to
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    // turn the IP into a URI
    char *captiveportal_uri = (char *)malloc(32 * sizeof(char));
    assert(captiveportal_uri && "Failed to allocate captiveportal_uri");
    strcpy(captiveportal_uri, "http://");
    strcat(captiveportal_uri, ip_addr);

    // get a handle to configure DHCP with
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    // set the DHCP option 114
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(netif));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, captiveportal_uri, strlen(captiveportal_uri)));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(netif));
}
#endif // CONFIG_ESP_ENABLE_DHCP_CAPTIVEPORTAL

esp_err_t wifi_manager_root_get_handler(httpd_req_t *req)
{
    const uint32_t root_len = root_end - root_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, root_start, root_len);

    return ESP_OK;
}

void url_decode(char *buf)
{
    char *src = buf;
    char *dst = buf;
    char a, b;
    while (*src)
    {
        if (*src == '+')
        {
            *dst = ' ';
            src++;
            dst++;
        }
        else if (*src == '%' && src[1] && src[2])
        {
            a = src[1];
            b = src[2];

            // Xử lý src[1]
            if (a >= '0' && a <= '9')
            {
                a -= '0';
            }
            else if (a >= 'a' && a <= 'f')
            {
                a = a - 'a' + 10;
            }
            else if (a >= 'A' && a <= 'F')
            {
                a = a - 'A' + 10;
            }

            // Xử lý src[2]
            if (b >= '0' && b <= '9')
            {
                b -= '0';
            }
            else if (b >= 'a' && b <= 'f')
            {
                b = b - 'a' + 10;
            }
            else if (b >= 'A' && b <= 'F')
            {
                b = b - 'A' + 10;
            }

            *dst = (a << 4) | b; // Kết hợp 2 hex thành 1 byte
            src += 3;
            dst++;
        }
        else
        {
            *dst = *src;
            src++;
            dst++;
        }
    }
    *dst = '\0';
}

esp_err_t wifi_manager_root_post_handler(httpd_req_t *req)
{
    char buf[256];                                        // ssid max là 32, password là 8-63 -> không cần xử lí trường hợp form data quá dài
    int ret = httpd_req_recv(req, buf, req->content_len); // Đọc data từ body request, lưu vào buf, return số byte đọc được từ body
    if (ret <= 0)
    {
        return ESP_FAIL;
    }
    buf[ret] = '\0'; // Thêm Null Terminator tạo thành 1 chuỗi, chuỗi có dạng: "ssid=abcdef&password=12345678"
    url_decode(buf);

    // Xử lí data buf, cắt chuỗi lấy ssid và password
    char *token = strtok(buf, "=&");
    int n = 0;
    char tmp[10][64];
    while (token != NULL)
    {
        strcpy(tmp[n], token);
        n++;
        token = strtok(NULL, "=&");
    }
    // Lưu vào post_data
    strncpy(post_data.sta_info.ssid, tmp[1], sizeof(post_data.sta_info.ssid) - 1);
    post_data.sta_info.ssid[sizeof(post_data.sta_info.ssid) - 1] = '\0';
    strncpy(post_data.sta_info.password, tmp[3], sizeof(post_data.sta_info.password) - 1);
    post_data.sta_info.password[sizeof(post_data.sta_info.password) - 1] = '\0';
    strncpy(post_data.server_ip, tmp[5], sizeof(post_data.server_ip) - 1);
    post_data.server_ip[sizeof(post_data.server_ip) - 1] = '\0';

    ESP_LOGI(TAG, "SSID: %s, Password: %s, Server IP: %s", post_data.sta_info.ssid, post_data.sta_info.password, post_data.server_ip);

    wifi_manager_nvs_save_config(&post_data); // Lưu data từ POST form vào NVS

    // Chuyển sang STA
    esp_wifi_stop();
    esp_wifi_deinit();
    wifi_manager_init_sta(&post_data.sta_info);

    return ESP_OK;
}

void wifi_manager_nvs_save_config(post_data_t *post_data)
{
    // Init NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NOT_ENOUGH_SPACE || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    nvs_handle_t my_nvs_handle;
    // ============================= Lưu STA config vào NVS ================================================
    // Open NVS
    ESP_LOGI(TAG_NVS, "Opening NVS handle...");
    err = nvs_open("STA_Mode", NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    // Store and WiFi Config to NVS
    ESP_LOGI(TAG_NVS, "Writing STA config to NVS...");
    err = nvs_set_str(my_nvs_handle, "SSID", post_data->sta_info.ssid);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Failed to write SSID");
    }
    err = nvs_set_str(my_nvs_handle, "Password", post_data->sta_info.password);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Failed to write Password");
    }
    // Commit changes
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    ESP_LOGI(TAG, "Committing updates in NVS...");
    err = nvs_commit(my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS changes!");
    }
    // Close
    nvs_close(my_nvs_handle);
    ESP_LOGI(TAG, "NVS handle closed.");
    // ============================= Lưu STA config vào NVS ================================================

    // ============================= Lưu Server IP vào NVS =================================================
    // Open NVS
    ESP_LOGI(TAG_NVS, "Opening NVS handle...");
    err = nvs_open("Server", NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    // Store and Server IP to NVS
    ESP_LOGI(TAG_NVS, "Writing Server IP to NVS...");
    err = nvs_set_str(my_nvs_handle, "IP", post_data->server_ip);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Failed to write Server IP");
    }
    // Commit changes
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    ESP_LOGI(TAG, "Committing updates in NVS...");
    err = nvs_commit(my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS changes!");
    }
    // Close
    nvs_close(my_nvs_handle);
    ESP_LOGI(TAG, "NVS handle closed.");
    // ============================= Lưu Server IP vào NVS =================================================
}

void wifi_manager_nvs_load_config(post_data_t *post_data)
{
    // Init NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NOT_ENOUGH_SPACE || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    nvs_handle_t my_nvs_handle;

    // ============================= Read STA config từ NVS ================================================
    // Open NVS
    ESP_LOGI(TAG_NVS, "Opening NVS handle...");
    err = nvs_open("STA_Mode", NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    // Read back the string
    size_t required_size = 0;
    ESP_LOGI(TAG, "Reading config from NVS...");
    err = nvs_get_str(my_nvs_handle, "SSID", NULL, &required_size);
    if (err == ESP_OK)
    {
        err = nvs_get_str(my_nvs_handle, "SSID", post_data->sta_info.ssid, &required_size);
    }
    err = nvs_get_str(my_nvs_handle, "Password", NULL, &required_size);
    if (err == ESP_OK)
    {
        err = nvs_get_str(my_nvs_handle, "Password", post_data->sta_info.password, &required_size);
    }
    ESP_LOGI(TAG, "Committing updates in NVS...");
    err = nvs_commit(my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS changes!");
    }
    // Close
    nvs_close(my_nvs_handle);
    ESP_LOGI(TAG, "NVS handle closed.");
    // =====================================================================================================

    // ============================= Read Server IP từ NVS =================================================
    // Open NVS
    ESP_LOGI(TAG_NVS, "Opening NVS handle...");
    err = nvs_open("Server", NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG_NVS, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    }
    // Read back the string
    ESP_LOGI(TAG, "Reading Server IP from NVS...");
    err = nvs_get_str(my_nvs_handle, "IP", NULL, &required_size);
    if (err == ESP_OK)
    {
        err = nvs_get_str(my_nvs_handle, "IP", post_data->server_ip, &required_size);
    }
    ESP_LOGI(TAG, "Committing updates in NVS...");
    err = nvs_commit(my_nvs_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to commit NVS changes!");
    }
    // Close
    nvs_close(my_nvs_handle);
    ESP_LOGI(TAG, "NVS handle closed.");
    // =====================================================================================================
}

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

httpd_handle_t wifi_manager_start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root_get);
        httpd_register_uri_handler(server, &root_post);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}
