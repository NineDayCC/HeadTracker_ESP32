#ifndef IO_H
#define IO_H

#include <zephyr/kernel.h>

extern struct k_sem btn1_single_click_sem;

bool isSingleClick(void);
bool isLongStart(void);

int io_Init(void);
void io_Thread(void);

void BTN1_SINGLE_Click_Handler(void* btn);
void BTN1_LONG_PRESS_START_Handler(void* btn);

#endif