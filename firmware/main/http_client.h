// C:\Users\hphuc\Desktop\http-prac
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_macros.h"

/**
 * @brief Structure storing HTTP client configuration and handle.
 * 
 * This struct contains the ESP-IDF HTTP client handle, server IP address,
 * and server port number used for sending HTTP requests.
 */
typedef struct {
    esp_http_client_handle_t client_handle;
    char server_ip[32];
    uint16_t server_port;
} http_client_t;

/**
 * @brief Initialize an HTTP client instance.
 * 
 * This function initializes an HTTP client and stores the handle, IP, and port 
 * into the provided @ref http_client_t structure. It should be called once 
 * before sending requests.
 * 
 * @param[out] client Pointer to an http_client_t structure to initialize.
 * @param[in] server_ip Server IP address (e.g., "192.168.1.10").
 * @param[in] port Server port number (e.g., 5000).
 * 
 * @return
 *  - ESP_OK on success  
 *  - ESP_FAIL if initialization fails
 */
extern esp_err_t http_client_init(http_client_t *client, char *server_ip, uint16_t port);

/**
 * @brief Send an HTTP POST request with JSON payload.
 * 
 * This function sends a POST request to the given path using the initialized 
 * HTTP client. The body of the request is the provided JSON string.
 * 
 * @param[in] client Pointer to a previously initialized http_client_t instance.
 * @param[in] path HTTP endpoint to post to (e.g., "/test_post").
 * @param[in] json_data JSON payload to send.
 * 
 * @return
 *  - ESP_OK on success  
 *  - ESP_FAIL if the POST request fails
 */
extern esp_err_t http_client_post(http_client_t *client, char *path, char *json_data);

#endif