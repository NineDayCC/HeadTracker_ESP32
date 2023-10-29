#pragma once

#include "ht.h"
#include "receiver.h"

#define IO_ACTIVE_LOW  0
#define IO_ACTIVE_HIGH 1
#define GPIO_CENTER_BUTTON_SET  GPIO_CENTER_BUTTON
#define GPIO_CENTER_BUTTON_ACTIVE_LEVEL IO_ACTIVE_LOW  //center button active when low

#define IO_THREAD_PRIORITY_SET      IO_THREAD_PRIORITY  //thread priority
#define IO_THREAD_STACK_SIZE_SET    IO_THREAD_STACK_SIZE  //thread stack size

bool isSingleClick(void);
bool isLongStart(void);

void io_Init(void);
void io_Thread(void *pvParameters);

void BTN1_SINGLE_Click_Handler(void* btn);
void BTN1_LONG_PRESS_START_Handler(void* btn);
uint8_t read_button_GPIO(uint8_t button_id);
