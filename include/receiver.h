#pragma once
#ifdef RECEIVER

#include "defines.h"

// Pins set

#ifdef RECEIVER_LAUT  // receiver Lite pin sets
#define GPIO_CENTER_BUTTON  GPIO_NUM_9
#define GPIO_PPM_OUT        GPIO_NUM_10

#endif



/**
 * @brief receiver main fucntion
 */
void receiver_start(void);

#endif