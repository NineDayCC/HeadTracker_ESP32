#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "ht.h"


static const char* MODE_TAG = "MODE";

void app_main()
{
    #ifdef HT_LITE
    ESP_LOGI(MODE_TAG, "HeadTracker Lite");
    #endif
    headtracker_start();
}
