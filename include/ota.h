#ifndef __OTA_H__
#define __OTA_H__

//C
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include <math.h>
//FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
//esp32
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "spi_flash_mmap.h"
#include "esp_timer.h"
#include "esp_ota_ops.h"
#include "esp_http_server.h"
#include "esp_app_format.h"

#include "lwip/err.h"
#include "lwip/sys.h"

void firmware_Sha256();
void HttpOTA_server_init();

#endif


