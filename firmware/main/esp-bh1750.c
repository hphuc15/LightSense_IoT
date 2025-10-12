#include <stdio.h>
#include <time.h>
#include "driver/i2c_master.h"
#include "bh1750.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "config.h"

static int s_retry_num = 0;
static EventGroupHandle_t s_wifi_event_group;

/**
 * @brief I2C Master Initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);

/**
 * @brief WiFi Event Handler Functions
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG_w, "WiFi started, connecting ...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < ESP_WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            ESP_LOGI(TAG_w, "Retry to connect (%d/%d)", s_retry_num, ESP_WIFI_MAX_RETRY);
            s_retry_num++;
        }
        else
        {
            ESP_LOGI(TAG_w, "Failed to connect after %d attempts", ESP_WIFI_MAX_RETRY);
            xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_w, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, ESP_WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief WiFi Initialization
 */
static esp_err_t wifi_init()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_driver_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_driver_cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_network_cfg = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PSW}};

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_network_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_w, "wifi_init finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, ESP_WIFI_CONNECTED_BIT | ESP_WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & ESP_WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_w, "Connected to SSID: %s", ESP_WIFI_SSID);
        return ESP_OK;
    }
    else if (bits & ESP_WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG_w, "Failed to connect to SSID: %s", ESP_WIFI_SSID);
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGI(TAG_w, "Unexpected event");
        return ESP_FAIL;
    }
}

// ============================== I2C+BH1750 Module Function ==========================
typedef struct {
    float light;
    char timestamp[30];
} data_send_t;

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
/***
 * @brief Convert data to JSON format Function, call this function after bh1750_get_data(). 
 * Add timestamp to JSON.
 * @param bh1750_data The sensor data just taken
 * @return JSON format
 */
char* data2json(float bh1750_data){
    time_t current_time;
    time(&current_time);

    data_send_t data_send;
    data_send.light = bh1750_data;
    strncpy(data_send.timestamp, ctime(&current_time), sizeof(data_send.timestamp)-1);
    data_send.timestamp[sizeof(data_send.timestamp)-1] = '\0'; // đảm bảo kết thúc chuỗi

    char post_data[50];
    snprintf(
        post_data,
        sizeof(post_data),
        "{\"light\": %.2f, \"timestamp\": %s}",
        data_send.light, 
        data_send.timestamp
    );
    return post_data;
}



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



    // ===================================== Main loop =======================================
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        ret = esp_wifi_connect();
        while(ret == ESP_FAIL){
            ESP_LOGE(TAG_w, "WiFi is disconnected");
            ESP_LOGI(TAG_w, "Attempting to connect AP");
            ret = esp_wifi_connect();
            if(ret == ESP_OK){
                break;
            }
        }
        esp_err_t bh1750_status = bh1750_get_data(bh1750_sensor, &bh1750_data);
        char *bh1750_data_json = data2json(bh1750_data);
        /*
        Hàm http gửi data dùng ở đây, sau khi dòng data2json(bh1750_data); chạy xong, dùng JSON bh1750_data_json để gửi
        */


        // Log data to Serial Monitor
        if (bh1750_status == ESP_OK)
        {
            ESP_LOGI(TAG_b, "Light: %.2f lux\n", bh1750_data);
        }
        else
        {
            printf("Error!\n");
        }
    }
}