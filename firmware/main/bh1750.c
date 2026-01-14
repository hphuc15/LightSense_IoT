/**
 * @file    bh1750.c
 * @brief   Implementation of BH1750 ambient light sensor driver
 */
#include "bh1750.h"

#define BH1750_CMD_POWER_DOWN 0x00
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_RESET 0x07

/**
 * @brief Measurement accuracy factor
 * @details Used to convert raw sensor data to lux value
 */
#define BH1750_MEASUREMENT_ACCURACY 1.2

static const char *TAG = "[bh1750]";

/**
 * @brief Initialize I2C master bus and add BH1750 device
 */
static void bh1750_i2c_master_init(BH1750_t *sensor)
{
    esp_err_t ret;
    ret = i2c_new_master_bus(&sensor->bus_cfg, &sensor->bus_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[i2c_new_master_bus] error: %s", esp_err_to_name(ret));
        return;
    }

    ret = i2c_master_bus_add_device(sensor->bus_handle, &sensor->dev_cfg, &sensor->dev_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[i2c_master_bus_add_device] error: %s", esp_err_to_name(ret));
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    ESP_LOGI(TAG, "I2C Initialized");
}

void BH1750_Init(BH1750_t *sensor)
{
    bh1750_i2c_master_init(sensor);
    esp_err_t ret;
    uint8_t cmd;

    // Power on
    cmd = BH1750_CMD_POWER_ON;
    ret = i2c_master_transmit(sensor->dev_handle, &cmd, sizeof(cmd), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[i2c_master_transmit] error: %s", esp_err_to_name(ret));
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    // Measurement mode
    cmd = sensor->mode;
    ret = i2c_master_transmit(sensor->dev_handle, &cmd, sizeof(cmd), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "[i2c_master_transmit] error: %s", esp_err_to_name(ret));
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "Sensor initialized");
}

void BH1750_Read(BH1750_t *sensor)
{
    uint8_t data[2];
    esp_err_t ret;

    ret = i2c_master_receive(sensor->dev_handle, data, sizeof(data), pdMS_TO_TICKS(1000));
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error in reading bh1750 data: %s", esp_err_to_name(ret));
        return;
    }
    sensor->data = ((data[0] << 8) | data[1]) / BH1750_MEASUREMENT_ACCURACY;
}

void BH1750_Data_To_Json(BH1750_t *sensor)
{
    cJSON *root = cJSON_CreateObject();
    if (sensor->id)
    {
        cJSON_AddNumberToObject(root, "sensor_id", sensor->id);
    }
    cJSON_AddNumberToObject(root, "light", sensor->data);

    if (sensor->data_json)
    {
        free(sensor->data_json);
        sensor->data_json = NULL;
    }
    sensor->data_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
}