#include "bh1750_sensor.h"

static const char *TAG_I2C = "I2C";

bh1750_handle_t bh1750_handle;
bh1750_data_t bh1750_data;

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t i2c_mst_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 1,
        .flags.enable_internal_pullup = true};
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_conf, bus_handle));

    i2c_device_config_t i2c_dev_conf = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = BH1750_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ};
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &i2c_dev_conf, dev_handle));
}

void bh1750_init(void)
{
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG_I2C, "I2C Initialized successfully");
    ESP_ERROR_CHECK(bh1750_create(bus_handle, BH1750_SENSOR_ADDR, &bh1750_handle));
    ESP_ERROR_CHECK(bh1750_power_on(bh1750_handle));
    vTaskDelay(pdMS_TO_TICKS(10));
    bh1750_set_measure_mode(bh1750_handle, BH1750_MEASUREMENT_MODE);
    vTaskDelay(pdMS_TO_TICKS(50));
}

void bh1750_get_json_string(bh1750_handle_t bh1750_handler, bh1750_data_t *bh1750_data, char *bh1750_json_data)
{
    // Light:
    bh1750_get_data(bh1750_handler, &bh1750_data->light);

    // Timestamp:
    time(&bh1750_data->timestamp);                      // Get current time than save to bh1750_data.timestamp
    char *timestamp = ctime(&bh1750_data->timestamp);   // Convert timestamp (time_t) to string
    timestamp[strcspn(timestamp, "\n")] = '\0';         // Removes the newline character \n at the end of the timestamp string

    // to json
    cJSON *json_data_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(json_data_obj, "sensor_id", SENSOR_ID);
    cJSON_AddNumberToObject(json_data_obj, "light", bh1750_data->light);
    cJSON_AddStringToObject(json_data_obj, "timestamp", timestamp);

    char *json_data_str = cJSON_PrintUnformatted(json_data_obj);
    strcpy(bh1750_json_data, json_data_str);

    free(json_data_str);
    cJSON_Delete(json_data_obj);
}