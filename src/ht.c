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
#include "app_espnow.h"
#include "ota.h"

void headtracker_start(void)
{
    firmware_Sha256();                  //校验当前固件
    io_Init();
    if (is_OTA_Mode())  //是否进入OTA模式
    {
        HttpOTA_server_init();          //OTA服务器初始化
    }
    else
    {
        imu_Init();
        ht_espnow_init();
    }

#ifdef HT_LITE
    PPMinit();
    bt_tx_init();
#elif HT_NANO
#endif
}

#endif