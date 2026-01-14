idf_component_register(SRCS "Button.c" "WiFiManager.c" "main.c" "bh1750.c" "HttpClient.c" 
                       PRIV_REQUIRES esp-tls nvs_flash esp_event esp_netif esp_http_client driver cJSON dns_server esp_wifi esp_http_server esp_timer
                       INCLUDE_DIRS "."
                       EMBED_FILES "root.html")
