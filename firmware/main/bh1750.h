/**
 * @file    bh1750.h
 * @brief   BH1750 ambient light sensor driver for ESP32
 * @author  hphuc15
 * @date    January 08, 2026
 * @version 1.0.0
 *
 * @details This driver provides functions to interface with the BH1750
 *          digital ambient light sensor via I2C communication. It supports
 *          various measurement modes and JSON data formatting.
 *
 * @note    Requires FreeRTOS and ESP-IDF framework
 * @warning Ensure I2C pins are properly configured before initialization
 */

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "cJSON.h"

/**
 * @brief Measurement mode enumeration
 */
typedef enum
{
    BH1750_MODE_CONTI_H_RES_1 = 0x10, /*!< Continuously H-Resolution Mode (1 lx, 120ms) */
    BH1750_MODE_CONTI_H_RES_2 = 0x11, /*!< Continuously H-Resolution Mode 2 (0.5 lx, 120ms) */
    BH1750_MODE_CONTI_L_RES = 0x13,   /*!< Continuously L-Resolution Mode (4 lx, 16ms) */
    BH1750_MODE_OT_H_RES_1 = 0x20,    /*!< One Time H-Resolution Mode (1 lx, 120ms, auto power down) */
    BH1750_MODE_OT_H_RES_2 = 0x21,    /*!< One Time H-Resolution Mode 2 (0.5 lx, 120ms, auto power down) */
    BH1750_MODE_OT_L_RES = 0x23       /*!< One Time L-Resolution Mode (4 lx, 16ms, auto power down) */
} BH1750_Mode_t;

/**
 * @brief BH1750 sensor handle structure
 */
typedef struct BH1750_t
{
    i2c_master_bus_config_t bus_cfg;    /*!< I2C bus configuration */
    i2c_device_config_t dev_cfg;        /*!< I2C device configuration */
    i2c_master_bus_handle_t bus_handle; /*!< I2C bus handle */
    i2c_master_dev_handle_t dev_handle; /*!< I2C device handle */

    int id;             /*!< Sensor identifier (optional, for multi-sensor systems) */
    BH1750_Mode_t mode; /*!< Current measurement mode */
    float data;         /*!< Latest light intensity reading in lux */
    char *data_json;    /*!< JSON formatted sensor data (dynamically allocated) */
} BH1750_t;

/**
 * @brief Initialize bh1750 function
 *
 * @param[in,out] sensor Pointer to BH1750_t structure with pre-configured
 */
void BH1750_Init(BH1750_t *sensor);

/**
 * @brief Read light intensity from BH1750 sensor
 *
 * Reads 2 bytes of data from the sensor and calculates light intensity
 * in lux. The result is stored in sensor->data.
 *
 * @param[in,out] sensor Pointer to BH1750_t structure with pre-configured
 */
void BH1750_Read(BH1750_t *sensor);

/**
 * @brief Convert sensor data to JSON format
 *
 * Creates a JSON object containing sensor ID (if set) and light intensity.
 * The JSON string is stored in sensor->data_json.
 */
void BH1750_Data_To_Json(BH1750_t *sensor);