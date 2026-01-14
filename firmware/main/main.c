#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "bh1750.h"
#include "HttpClient.h"
#include "WiFiManager.h"
#include "Config.h"
#include "Button.h"

static const char *Tag = "[Main]";
static char des_server_ip[64];

static WiFiManager_t wm;

static HttpClient_t client;
static HttpClient_Config_t cli_config = {
    .host = DROGON_SERVER_HOST,
    .port = DROGON_SERVER_PORT,
    .path = DROGON_SERVER_PATH_DEFAULT,
    .transport_type = HTTP_TRANSPORT_OVER_TCP};

static BH1750_t sensor = {
    .id = 1,
    .bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 1,
        .flags.enable_internal_pullup = true},
    .dev_cfg = {.dev_addr_length = I2C_ADDR_BIT_7, .device_address = BH1750_SLAVE_ADDR, .scl_speed_hz = I2C_MASTER_FREQ_HZ},
    .mode = BH1750_MODE_CONTI_H_RES_1};

// Send sensor data to endpoint of server each SensorDalayMs ms
static void Send(int SensorDelayMs)
{
    BH1750_Read(&sensor);
    BH1750_Data_To_Json(&sensor);
    HttpClient_Post(&client, DROGON_SERVER_PATH_RECV_DATA, sensor.data_json, "application/json");
    vTaskDelay(pdMS_TO_TICKS(SensorDelayMs));
}

void LedCallback()
{
    while (1)
    {
        EventBits_t bits = xEventGroupWaitBits(wm.event.group, ESP_WIFI_EVENT_BIT_STACONNECTED | ESP_WIFI_EVENT_BIT_APSTART, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000));

        if (bits & ESP_WIFI_EVENT_BIT_STACONNECTED)
        {
            gpio_set_level(GPIO_NUM_2, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else if (bits & ESP_WIFI_EVENT_BIT_APSTART)
        {
            gpio_set_level(GPIO_NUM_2, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(GPIO_NUM_2, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
        else
        {
            gpio_set_level(GPIO_NUM_2, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
void Led_Init()
{
    gpio_config_t led_gpio = {
        .pin_bit_mask = 1ULL << LED_GPIO_NUM,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&led_gpio);
    gpio_set_level(LED_GPIO_NUM, 0);
    xTaskCreate(LedCallback, "LedTask", 2048, NULL, 5, NULL);
}

// Isr func call when button pressed 3s
static void IsrCallback()
{
    HttpClient_Deinit(&client);
    wm.config.ap = ESP_WIFI_AP_CONFIG_DEFAULT();
    WiFiManager_STA_ConfigViaAP(&wm);
    // Get destination server configuration from NVS
    if (WiFiManager_GetBoolJSON(wm.portal.json, "option", "enabled", false))
    {
        char new_ip[64];
        char new_port[6];
        WiFiManager_JSON_GetValue(wm.portal.json, "option", "server_ip", new_ip, sizeof(new_ip));
        WiFiManager_JSON_GetValue(wm.portal.json, "option", "server_port", new_port, sizeof(new_port));
        WiFiManager_NVS_SetValue("server", "server_ip", new_ip);
        WiFiManager_NVS_SetValue("server", "server_port", new_port);
        int new_port_int = strtol(new_port, NULL, 10);
        if (new_ip[0] == '\0' || new_port_int <= 0 || new_port_int > 65535)
        {
            strcpy(des_server_ip, DROGON_SERVER_HOST);
            new_port_int = DROGON_SERVER_PORT;
        }
        else
        {
            strcpy(des_server_ip, new_ip);
        }
        cli_config.host = des_server_ip;
        cli_config.port = new_port_int;
    }

    HttpClient_Init(&client, cli_config);
    ESP_LOGI("[DEBUG]", "Server config: server_ip: %s, server_port(int): %d", cli_config.host, cli_config.port);
}

// Check if STA start
bool IsStaActive()
{
    return WiFiManager_IsStaActive(&wm);
}

// Init wifi and des server with configuration that storaged in NVS, then init HttpClient and sensor
static void Init(void)
{
    // Init WiFi
    WiFiManager_NVS_GetSTA(&wm);
    WiFiManager_Init(&wm);
    WiFiManager_STA_Start(&wm);
    // Get destination server configuration from NVS
    char new_ip[64];
    char new_port[6];
    WiFiManager_NVS_GetValue("server", "server_ip", new_ip, sizeof(new_ip));
    WiFiManager_NVS_GetValue("server", "server_port", new_port, sizeof(new_port));
    int new_port_int = strtol(new_port, NULL, 10);
    if (new_ip[0] == '\0' || new_port_int <= 0 || new_port_int > 65535)
    {
        strcpy(des_server_ip, DROGON_SERVER_HOST);
        new_port_int = DROGON_SERVER_PORT;
    }
    else
    {
        strcpy(des_server_ip, new_ip);
    }
    cli_config.host = des_server_ip;
    cli_config.port = new_port_int;
    // Save server configuration for next time
    snprintf(new_port, sizeof(new_port), "%d", cli_config.port);
    WiFiManager_NVS_SetValue("server", "server_ip", cli_config.host);
    WiFiManager_NVS_SetValue("server", "server_port", new_port);

    HttpClient_Init(&client, cli_config);
    BH1750_Init(&sensor);
    Led_Init();
}

void app_main(void)
{
    ButtonInit(IsrCallback);
    nvs_flash_init();
    esp_netif_init();

    Init();

    vTaskDelay(pdMS_TO_TICKS(2000));
    while (1)
    {
        if (IsStaActive())
        {
            Send(5000);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}