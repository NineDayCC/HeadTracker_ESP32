#ifndef FRAMEWORK_ARDUINO
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "ht.h"
#include "receiver.h"

static const char *MODE_TAG = "MODE";

void app_main()
{
    uart_set_baudrate(UART_NUM_0, 921600);
#ifdef HEADTRACKER
    ESP_LOGI(MODE_TAG, "HeadTracker");
    headtracker_start();
#elif defined RECEIVER
    ESP_LOGI(MODE_TAG, "Receiver");
    receiver_start();
#endif
}
#endif