# WIFI MODULE
## Functions
### ```void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)```
This Function use to handle with STA or AP event that include:
- ```WIFI_EVENT_STA_START```:
- ```WIFI_EVENT_STA_DISCONNECTED```:
- ```IP_EVENT_STA_GOT_IP```:
- ```WIFI_EVENT_AP_STADISCONNECTED```:
- ```WIFI_EVENT_AP_STACONNECTED```:




### ```void wifi_init_sta(char *sta_ssid, char *sta_password)```
### ```void wifi_init_softap(void)```
### ```void dhcp_set_captiveportal_url(void)```
### ```esp_err_t root_get_handler(httpd_req_t *req)```
### ```void url_decode(char *buf)```

### ```esp_err_t root_post_handler(httpd_req_t *req)```

### ```esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)```

### ```httpd_handle_t start_webserver(void)```



Viết vô cách cài đặt:
```bash
idf.py add-dependency "espressif/bh1750^2.0.0"
idf.py add-dependency "espressif/cjson^1.7.19"
```