#pragma once
#ifdef RECEIVER

#include "defines.h"

// Pins set

#ifdef RECEIVER_LAUT  // receiver Lite pin sets
#define GPIO_CENTER_BUTTON  GPIO_NUM_9
#define GPIO_PPM_OUT        GPIO_NUM_10
#define GPIO_LED_STATUS     GPIO_NUM_12
#define GPIO_BT_STATUS      GPIO_NUM_13

#endif



/**
 * @brief receiver main fucntion
 */
void receiver_start(void);

#endif