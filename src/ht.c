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
    PPMinit();
    imu_Init();
    bt_ht_init();
}