#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "ht.h"
#include "receiver.h"

static const char* MODE_TAG = "MODE";

void app_main()
{
    #ifdef HT_LITE
    ESP_LOGI(MODE_TAG, "HeadTracker Lite");
    headtracker_start();
    #elif defined RECEIVER_LAUT
    ESP_LOGI(MODE_TAG, "Receiver Luat board");
    receiver_start();
    #endif
}
