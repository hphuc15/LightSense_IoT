#ifndef BH1750_SENSOR_H
#define BH1750_SENSOR_H

#include <cJSON.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "bh1750.h"
#include "time.h"

#define I2C_MASTER_SDA_IO               21     // GPIO for I2C SDA
#define I2C_MASTER_SCL_IO               22     // GPIO for I2C SCL
#define I2C_MASTER_NUM                  I2C_NUM_0 // I2C port number for master
#define I2C_MASTER_FREQ_HZ 100000

#define BH1750_SENSOR_ADDR              BH1750_I2C_ADDRESS_DEFAULT   // Address of the BH1750 sensor
#define BH1750_MEASUREMENT_MODE         BH1750_CONTINUE_1LX_RES // Measurement mode of BH1750 sensor

extern i2c_master_bus_handle_t bus_handle;
extern i2c_master_dev_handle_t dev_handle;

typedef struct {
    float light;
    time_t timestamp;
} bh1750_data_t;
extern bh1750_data_t bh1750_data; // BH1750 data struct

extern bh1750_handle_t bh1750_handle; // BH1750 handler
extern char bh1750_json_data[]; // BH1750 json data

/**
 * @brief BH1750 Initialize Function
 */
extern void bh1750_init(void);

/**
 * @brief Get BH1750 data and stick timestamp when take the data then convert them to JSON format.
 * 
 * @param *bh1750_handler BH1750 sensor handle
 * @param *bh1750_data Struct to storage BH1750 data and timestamp
 * @param *bh1750_json_data Variable to storage JSON string after converted
 */
extern void bh1750_get_json_string(bh1750_handle_t bh1750_handler, bh1750_data_t *bh1750_data, char *bh1750_json_data);

#endif