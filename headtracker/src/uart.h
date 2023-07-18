#ifndef UART_H
#define UART_H

#include <zephyr/kernel.h>
#include "defines.h"

void uart_init();
void uart_Thread();

#endif