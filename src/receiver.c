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

void receiver_start(void)
{
    PPMinit();

#ifdef RECEIVER_LAUT
    bt_rx_init();
#endif
}
#endif

#endif