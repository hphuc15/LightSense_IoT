#ifndef CONFIG_H
#define CONFIG_H

// BH1750 Configuration
#define I2C_MASTER_SCL_IO       GPIO_NUM_22     /*!< GPIO number for I2C master clock */
#define I2C_MASTER_SDA_IO       GPIO_NUM_21     /*!< GPIO number for I2C master data */
#define I2C_MASTER_NUM          I2C_NUM_0       /*!< I2C port number for master device */
#define I2C_MASTER_FREQ_HZ      100000          /*!< I2C master clock frequency in Hz */
#define I2C_MASTER_TIMEOUT_MS   1000            /*!< I2C timeout in milliseconds */
#define BH1750_SLAVE_ADDR       0x23            /*!< I2C address of BH1750 sensor (ADDR pin = LOW) */
// HttpClient Configuration
#define DROGON_SERVER_HOST "192.168.1.9"
#define DROGON_SERVER_PORT 5000
#define DROGON_SERVER_PATH_DEFAULT ""
#define DROGON_SERVER_PATH_RECV_DATA "/api/data/postData"
// WIFI LED state GPIO
#define LED_GPIO_NUM GPIO_NUM_2




#endif