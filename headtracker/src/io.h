#ifndef IO_H
#define IO_H

#include <zephyr/kernel.h>

extern struct k_sem button_pressed_sem;

void pressButton();

// Reset Button Pressed Flag on Read
bool wasButtonPressed();

int io_Init(void);
void io_Thread(void);

#endif