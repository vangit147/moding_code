#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "freertos/event_groups.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "data_flash.h"


esp_err_t _http_event_handler(esp_http_client_event_t *evt);

int http_test_task(char *dpwn_url);

#endif
