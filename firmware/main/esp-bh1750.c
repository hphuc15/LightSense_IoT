#include <stdio.h>
#include "driver/i2c_master.h"
#include "bh1750.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "config.h"


#define ESP_MAXIMUM_RETRY 3

static EventGroupHandle_t s_wifi_event_group; // con trỏ đến wifi event group
static int s_retry_num = 0;


/**
 * @brief I2C Master Initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);

/**
 * @brief WiFi Event Handler Functions
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
/**
 * @brief WiFi Initialization
 */
static void wifi_init();



void app_main(void)
{
// ======================================== WIFI ======================================
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    // Initialize WiFi
    wifi_init();

// ================================== BH1750 + I2C ==================================
    // I2C Initialize
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG_i, "I2C initialized succesfully!");
    // Sensor Initialize
    float bh1750_data;
    bh1750_handle_t bh1750_sensor;
    ESP_ERROR_CHECK(bh1750_create(bus_handle, BH1750_SENSOR_ADDR, &bh1750_sensor));
    ESP_ERROR_CHECK(bh1750_power_on(bh1750_sensor));
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(bh1750_set_measure_mode(bh1750_sensor, BH1750_MEASUREMENT_MODE));
    vTaskDelay(pdMS_TO_TICKS(180));
    while (1)
    {
        esp_err_t bh1750_status = bh1750_get_data(bh1750_sensor, &bh1750_data);
        if (bh1750_status == ESP_OK)
        {
            ESP_LOGI(TAG_b, "Light: %.2f lux\n", bh1750_data);
        }
        else
        {
            printf("Error!\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


// ================================= WiFI Module Function =============================
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) // WIFI_EVENT_STA_START: WiFi Station Interface has been succesfully started and commands are ready to receive.
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG_w, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG_w, "connect to the AP fail");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data; // ép kiểu event_data từ void* thành ip_event_got_ip_t* gán vào event (biến gốc event_data không đổi kiểu, vẫn là void*)
        ESP_LOGI(TAG_w, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t wifi_driver_cfg = WIFI_INIT_CONFIG_DEFAULT(); // WiFi driver config
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_driver_cfg));


    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));


    wifi_config_t wifi_network_cfg = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PSW,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD
        }
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_network_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_w, "wifi_init finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if(bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_w, "connected to ap SSID: %s, password: %s", ESP_WIFI_SSID, ESP_WIFI_PSW);
    }
    else if(bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG_w, "failed to connect to SSID: %s, password: %s", ESP_WIFI_SSID, ESP_WIFI_PSW);
    }
    else{
        ESP_LOGE(TAG_w, "UNEXPECTED EVENT");
    }
}

// ============================== I2C+BH1750 Module Function ==========================
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true};
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BH1750_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ};
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}