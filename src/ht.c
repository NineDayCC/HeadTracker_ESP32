#ifdef HEADTRACKER

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "ht.h"
#include "io.h"
#include "bt.h"
#include "imu.h"
#include "ppm.h"
#include "touch.h"
#include "buzzer.h"
#include "app_espnow.h"
#include "ota.h"
#include "mode.h"

void headtracker_start(void)
{
    firmware_Sha256(); // Verify current firmware

    io_Init();
    mode_init();       // Check bind mode and OTA mode

    imu_Init();
    ht_espnow_init();

#ifdef HT_LITE
    PPMinit();
    bt_tx_init();
#elif HT_NANO
#endif
}

#endif