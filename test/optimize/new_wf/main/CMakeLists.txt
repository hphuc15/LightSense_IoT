idf_component_register(SRCS "main.c" "wifi_m.c"
                       PRIV_REQUIRES esp_wifi nvs_flash esp_http_server dns_server cJSON driver esp_event
                       INCLUDE_DIRS ""
                       EMBED_FILES "root.html")
