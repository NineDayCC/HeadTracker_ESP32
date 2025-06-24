#ifdef RECEIVER
#ifndef FRAMEWORK_ARDUINO

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "receiver.h"
#include "io.h"
#include "ppm.h"
#ifdef RECEIVER_LAUT
#include "bt.h"
#endif
#ifdef RX_SE
#include "mode.h"
#include "app_espnow.h"
#endif

void receiver_start(void)
{
    io_Init();
    mode_init();

    PPMinit();
    rx_espnow_init();
#ifdef RECEIVER_LAUT
    bt_rx_init();
#endif
}
#endif

#endif