#pragma once

#include "ht.h"
#include "receiver.h"

#define IO_ACTIVE_LOW  0
#define IO_ACTIVE_HIGH 1
#define GPIO_CENTER_BUTTON_SET  GPIO_CENTER_BUTTON
#define GPIO_CENTER_BUTTON_ACTIVE_LEVEL         IO_ACTIVE_LOW  //center button active when low
#define GPIO_LED_STATUS_SET     GPIO_LED_STATUS
#define GPIO_LED_STATUS_SET_ACTIVE_LEVEL        IO_ACTIVE_LOW
#define GPIO_BT_STATUS_SET     GPIO_BT_STATUS
#define GPIO_BT_STATUS_SET_ACTIVE_LEVEL        IO_ACTIVE_LOW
#define GPIO_BUZZER_SET     GPIO_BUZZER
#define GPIO_BUZZER_ACTIVE_LEVEL        IO_ACTIVE_LOW
#define GPIO_OTA_BUTTON_SET  GPIO_OTA_BUTTON
#define GPIO_OTA_BUTTON_ACTIVE_LEVEL         IO_ACTIVE_LOW  //OTA button active when low


#define IO_THREAD_PRIORITY_SET      IO_THREAD_PRIORITY  //thread priority
#define IO_THREAD_STACK_SIZE_SET    IO_THREAD_STACK_SIZE  //thread stack size

bool isSingleClick(void);
bool isLongStart(void);

void io_Init(void);
void io_Thread(void *pvParameters);

void led_bt_ctrl(uint8_t status);
