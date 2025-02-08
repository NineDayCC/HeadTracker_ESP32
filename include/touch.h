#pragma once
#include "driver/touch_pad.h"

#define PIN_TOUCH           TOUCH_PAD_NUM7 // GPIO27
#define TOUCH_TRESHOULD     (325 - 50)// touch trigger threshould, depend on the board

void touch_Init(void);