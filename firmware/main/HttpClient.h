/**
 * @file    HttpClient.h
 * @brief   HTTP Client wrapper for ESP-IDF framework
 * @author  hphuc15
 * @date    January 09, 2026
 * @version 1.0.0
 *
 * @details This module provides a simplified interface for HTTP client operations
 *          on ESP32 devices using the ESP-IDF framework. It wraps the esp_http_client
 *          API with additional features like automatic buffer management and error handling.
 *
 * @note    Requires ESP-IDF framework
 * @warning Ensure network connectivity is established before using HTTP functions
 */

#ifndef HTTPCLIENT_H
#define HTTPCLIENT_H

#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_event.h"

#define HTTP_HEADER_CONTENT_TYPE "Content-Type"   /**< Content-Type header */
#define HTTP_HEADER_AUTHORIZATION "Authorization" /**< Authorization header */
#define HTTP_HEADER_ACCEPT "Accept"               /**< Accept header */
#define HTTP_HEADER_USER_AGENT "User-Agent"       /**< User-Agent header */

/**
 * @typedef HttpClient_Handle_t
 * @brief   HTTP client handle type
 * @details Wrapper for ESP-IDF HTTP client handle
 */
typedef esp_http_client_handle_t HttpClient_Handle_t;

/**
 * @typedef HttpClient_Transport_t
 * @brief   HTTP transport protocol type
 * @details Defines transport layer (HTTP or HTTPS)
 */
typedef esp_http_client_transport_t HttpClient_Transport_t;

/**
 * @typedef HttpClient_Event_Handle_Cb
 * @brief   HTTP event handler callback function pointer
 * @details User-defined callback for custom event handling
 * @param   evt Pointer to HTTP client event structure
 * @return  ESP_OK on success, error code otherwise
 */
typedef esp_err_t (*HttpClient_Event_Handle_Cb)(esp_http_client_event_t *evt);

/**
 * @struct  HttpClient_Config_t
 * @brief   HTTP client configuration structure
 * @details Contains all parameters needed to initialize an HTTP client connection
 */
typedef struct HttpClient_Config_t
{
    const char *host;                         /**< HTTP server hostname or IP address */
    int port;                                 /**< HTTP server port number */
    const char *path;                         /**< Default request path (e.g., "/api/v1") */
    HttpClient_Transport_t transport_type;    /**< Transport protocol type (HTTP/HTTPS) */
    HttpClient_Event_Handle_Cb event_handler; /**< Optional custom event handler callback */
} HttpClient_Config_t;

/**
 * @struct  HttpClient_t
 * @brief   HTTP client instance structure
 * @details Maintains the state and buffers for an HTTP client session
 */
typedef struct HttpClient_t
{
    HttpClient_Handle_t handle; /**< ESP-IDF HTTP client handle */

    char *rx_buffer;            /**< Pointer to receive buffer for response data */
    size_t rx_len;              /**< Current length of received data in buffer */
    size_t rx_size;             /**< Total allocated size of receive buffer */

    int last_status;            /**< Last HTTP response status code (e.g., 200, 404) */
    esp_err_t last_error;       /**< Last error code from HTTP operations */
} HttpClient_t;

/**
 * @brief   Initialize the HTTP client
 * 
 * @details This function initializes an HTTP client instance with the provided
 *          configuration parameters. It sets up the internal event handler and
 *          prepares the client for HTTP operations.
 * 
 * @param[out] client Pointer to HttpClient_t structure to initialize
 * @param[in]  config Configuration parameters for the HTTP client
 * 
 * @note    Must be called before any HTTP operations
 * @note    The client handle will be created and stored in the client structure
 * @warning Ensure all config parameters are valid before calling
 * 
 * @code
 * HttpClient_t client;
 * HttpClient_Config_t config = {
 *     .host = "api.example.com",
 *     .port = 443,
 *     .path = "/",
 *     .transport_type = HTTP_TRANSPORT_OVER_SSL,
 *     .event_handler = NULL
 * };
 * HttpClient_Init(&client, config);
 * @endcode
 */
void HttpClient_Init(HttpClient_t *client, HttpClient_Config_t config);

/**
 * @brief   Perform an HTTP POST request
 * 
 * @details Sends an HTTP POST request with the specified data to the given path.
 *          The function automatically sets the Content-Type header and handles
 *          the request/response cycle.
 * 
 * @param[in] client       Pointer to initialized HttpClient_t structure
 * @param[in] des_path     Destination URL path for the POST request
 * @param[in] post_data    Data to send in the POST request body
 * @param[in] content_type MIME type of the POST data (e.g., "application/json")
 * 
 * @return  void
 * 
 * @note    The function logs the HTTP status code and content length on success
 * @note    Errors are logged but the function does not return error codes
 * @warning client must be initialized with HttpClient_Init() before calling
 * @warning content_type must not be NULL or empty string
 * 
 * @code
 * const char *json_data = "{\"temperature\":25.5}";
 * HttpClient_Post(&client, "/api/sensor", json_data, "application/json");
 * @endcode
 */
void HttpClient_Post(HttpClient_t *client, const char *des_path, const char *post_data, const char *content_type);

/**
 * @brief   Deinitialize and cleanup the HTTP client
 * 
 * @details Releases all resources associated with the HTTP client, including
 *          the underlying ESP-IDF HTTP client handle. After calling this function,
 *          the client structure should not be used until reinitialized.
 * 
 * @param[in] client Pointer to HttpClient_t structure to cleanup
 * 
 * @note    This function should be called when the HTTP client is no longer needed
 * @note    Sets the client handle to NULL after cleanup
 * @warning Do not use the client after calling this function without reinitializing
 * 
 * @code
 * HttpClient_Deinit(&client);
 * @endcode
 */
void HttpClient_Deinit(HttpClient_t *client);

#endif