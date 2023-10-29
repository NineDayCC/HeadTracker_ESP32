#ifdef RECEIVER
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "receiver.h"
#include "io.h"
#include "bt.h"
#include "imu.h"
#include "ppm.h"

void receiver_start(void)
{
    io_Init();
    PPMinit();
    bt_rx_init();
}

#endif