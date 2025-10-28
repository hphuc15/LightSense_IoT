// C:\Users\hphuc\Desktop\http-prac
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_macros.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

#define SERVER_IP "172.20.10.4" // tạm
#define SERVER_PORT 5000



#endif