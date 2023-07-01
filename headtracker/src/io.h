#ifndef IO_H
#define IO_H

#include <zephyr/kernel.h>

extern struct k_sem button_pressed_sem;

void pressButton();

// Reset Button Pressed Flag on Read
bool wasButtonPressed();

int io_Init(void);
void io_Thread(void);

//test
#define LED_RUNNING (1 << 0)
#define LED_GYROCAL (1 << 1)
#define LED_BTCONNECTED (1 << 2)
#define LED_BTSCANNING (1 << 3)
#define LED_MAGCAL (1 << 4)
#define LED_BTCONFIGURATOR (1 << 5)

#endif