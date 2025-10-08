#pragma once

// =========================== WIFI CONFIG ==============================
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define ESP_WIFI_SSID "B9 106"
#define ESP_WIFI_PSW "B91062005@"
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK


// =========================== I2C CONFIG ================================
#define I2C_MASTER_SDA_IO 21     // GPIO for I2C SDA
#define I2C_MASTER_SCL_IO 22     // GPIO for I2C SCL
#define I2C_MASTER_NUM I2C_NUM_0 // I2C port number for master
#define I2C_MASTER_FREQ_HZ 100000


// ========================= BH1750 CONFIG ===============================
#define BH1750_SENSOR_ADDR BH1750_I2C_ADDRESS_DEFAULT   // Address of the BH1750 sensor
#define BH1750_MEASUREMENT_MODE BH1750_CONTINUE_1LX_RES // Measurement mode of BH1750 sensor


// =========================== LOG TAGS ==================================
static const char *TAG_i = "i2c";
static const char *TAG_b = "bh1750";
static const char *TAG_w = "wifi"; // WiFi Tag