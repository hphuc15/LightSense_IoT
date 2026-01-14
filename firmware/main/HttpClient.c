/**
 * @file    HttpClient.c
 * @brief   Implementation of HTTP Client wrapper for ESP-IDF
 * @author  hphuc15
 * @date    January 09, 2026
 * @version 1.0.0
 *
 * @details This file implements the HTTP client functionality declared in HttpClient.h.
 *          It provides wrapper functions around ESP-IDF's HTTP client API with
 *          additional error handling, logging, and buffer management capabilities.
 *
 * @warning Ensure proper error handling when using these functions
 */

#include "HttpClient.h"

const char *Tag = "[HttpClient]";

/**
 * @brief Internal HTTP event handler
 *
 * @param[in] evt Pointer to HTTP client event structure containing event data
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    HttpClient_t *cli = (HttpClient_t *)evt->user_data;

    if (!cli)
        return ESP_FAIL;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            size_t copy_len;
            if (evt->data_len < (cli->rx_size - cli->rx_len))
            {
                copy_len = evt->data_len;
            }
            else
            {
                copy_len = cli->rx_size - cli->rx_len;
            }

            memcpy(cli->rx_buffer + cli->rx_len,
                   evt->data,
                   copy_len);

            cli->rx_len += copy_len;
        }
        break;

    case HTTP_EVENT_ON_FINISH:
        cli->last_status =
            esp_http_client_get_status_code(evt->client);
        break;

    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(Tag, "HTTP redirect");
        break;

    case HTTP_EVENT_ERROR:
        cli->last_error = ESP_FAIL;
        break;

    default:
        break;
    }

    return ESP_OK;
}

/* ============================================================================
 * PUBLIC API IMPLEMENTATION
 * ============================================================================ */

void HttpClient_Init(HttpClient_t *client, HttpClient_Config_t config)
{
    esp_http_client_config_t client_config = {
        .host = config.host,
        .port = config.port,
        .path = config.path,
        .transport_type = config.transport_type,
        .event_handler = http_event_handler};
    client->handle = esp_http_client_init(&client_config);

    ESP_LOGI(Tag, "Http Client Initialized");
}

void HttpClient_Post(HttpClient_t *client, const char *des_path, const char *post_data, const char *content_type)
{
    esp_err_t ret;

    if (client == NULL || client->handle == NULL)
    {
        ESP_LOGE(Tag, "Invalid Handle Argument");
        return;
    }

    if (content_type == NULL || content_type[0] == '\0')
    {
        ESP_LOGE(Tag, "Invalid Content-Type");
        return;
    }

    esp_http_client_set_url(client->handle, des_path);
    esp_http_client_set_header(client->handle, HTTP_HEADER_CONTENT_TYPE, content_type);
    esp_http_client_set_method(client->handle, HTTP_METHOD_POST);
    esp_http_client_set_post_field(client->handle, post_data, strlen(post_data));

    ret = esp_http_client_perform(client->handle);
    if (ret == ESP_OK)
    {
        ESP_LOGI(Tag, "HTTP POST Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client->handle),
                 esp_http_client_get_content_length(client->handle));
    }
    else
    {
        ESP_LOGE(Tag, "HTTP POST request failed: %s", esp_err_to_name(ret));
    }
}

void HttpClient_Deinit(HttpClient_t *client)
{
    if (client && client->handle)
    {
        esp_http_client_cleanup(client->handle);
        client->handle = NULL;
    }
}