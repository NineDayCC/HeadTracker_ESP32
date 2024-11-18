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

void headtracker_start(void)
{
    io_Init();
    imu_Init();
#ifdef HT_LITE
    PPMinit();
    bt_tx_init();
#endif
}

#endif