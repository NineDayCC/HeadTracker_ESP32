#ifdef HEADTRAKCER

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
#include "espnow.h"

void headtracker_start(void)
{
    io_Init();
    // is bind button is active when start, enter bind mode.
    imu_Init();
    ht_espnow_init();
#ifdef HT_LITE
    PPMinit();
    bt_tx_init();
#elif HT_NANO
#endif
}

#endif