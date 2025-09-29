#include <stdio.h>
#include "driver/i2c_master.h"
#include "bh1750.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define I2C_MASTER_SDA_IO 21     // GPIO number for I2C master clock
#define I2C_MASTER_SCL_IO 22     // GPIO number for I2C master data
#define I2C_MASTER_NUM I2C_NUM_0 // I2C port number for master
#define I2C_MASTER_FREQ_HZ 100000

#define BH1750_SENSOR_ADDR BH1750_I2C_ADDRESS_DEFAULT   // Address of the BH1750 sensor
#define BH1750_MEASUREMENT_MODE BH1750_CONTINUE_1LX_RES // Measurement mode of BH1750 sensor

static const char *TAG_I2C = "i2c";
static const char *TAG_BH1750 = "bh1750";

/**
 * @brief I2C Master Initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG_I2C, "I2C initialized succesfully!");

    float bh1750_data;
    bh1750_handle_t bh1750_sensor;
    ESP_ERROR_CHECK(bh1750_create(bus_handle, BH1750_SENSOR_ADDR, &bh1750_sensor));
    ESP_ERROR_CHECK(bh1750_power_on(bh1750_sensor));
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(bh1750_set_measure_mode(bh1750_sensor, BH1750_MEASUREMENT_MODE));
    vTaskDelay(pdMS_TO_TICKS(180));
    while(1)
    {
        esp_err_t bh1750_status = bh1750_get_data(bh1750_sensor, &bh1750_data);
        if(bh1750_status == ESP_OK)
        {
            ESP_LOGI(TAG_BH1750, "Light: %.2f lux\n", bh1750_data);
        }
        else{
            printf("Error!\n");
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

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
