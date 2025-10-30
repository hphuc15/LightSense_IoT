// C:\Users\hphuc\Desktop\http-prac
#include "http_client.h"

const char *TAG_HTTP = "HTTP_CLIENT";

static esp_err_t http_client_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG_HTTP, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG_HTTP, "Connected");
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG_HTTP, "Received %d bytes", evt->data_len);
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_HTTP, "Disconnected");
        break;
    default:
        break;
    }
    return ESP_OK;
}

esp_err_t http_client_init(http_client_t *client, char *server_ip, uint16_t port)
{
    strncpy(client->server_ip, server_ip, sizeof(client->server_ip) - 1);
    client->server_ip[sizeof(client->server_ip) - 1] = '\0';
    client->server_port = port;

    char url[128];
    sprintf(url, "http://%s:%d", client->server_ip, client->server_port);
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_client_event_handler};

    client->client_handle = esp_http_client_init(&config);

    if (client->client_handle == NULL)
    {
        ESP_LOGE(TAG_HTTP, "Failed to init HTTP client for %s", url);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG_HTTP, "HTTP client initialized for %s:%d", client->server_ip, client->server_port);
    return ESP_OK;
}

esp_err_t http_client_post(http_client_t *client, char *path, char *json_data)
{
    if (client == NULL || client->client_handle == NULL)
    {
        ESP_LOGE(TAG_HTTP, "Client handle is NULL, cannot perform POST");
        return ESP_FAIL;
    }

    char url[128];
    snprintf(url, sizeof(url), "http://%s:%d/%s", client->server_ip, client->server_port, path);
    esp_http_client_set_url(client->client_handle, url);
    esp_http_client_set_method(client->client_handle, HTTP_METHOD_POST);
    esp_http_client_set_header(client->client_handle, "Content-Type", "Application/json");
    esp_http_client_set_post_field(client->client_handle, json_data, strlen(json_data));

    esp_err_t err = esp_http_client_perform(client->client_handle);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG_HTTP, "POST %s -> Status: %d", url, esp_http_client_get_status_code(client->client_handle));
    }
    else
    {
        ESP_LOGE(TAG_HTTP, "POST failed: %s", esp_err_to_name(err));
    }

    return err;
}
