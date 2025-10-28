idf_component_register(SRCS main.c wifi_manager.c button_handler.c led_handler.c bh1750_sensor.c http_client
                       PRIV_REQUIRES esp_event nvs_flash esp_wifi esp_http_server dns_server esp_driver_gpio esp_http_client bh1750
                       EMBED_FILES root.html)
