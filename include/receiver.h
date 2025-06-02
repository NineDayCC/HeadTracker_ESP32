#pragma once
#ifdef RECEIVER

#include "defines.h"

// Pins set

#ifdef RECEIVER_LAUT  // receiver Lite pin sets
#define GPIO_CENTER_BUTTON  GPIO_NUM_9
#define GPIO_PPM_OUT        GPIO_NUM_10
#define GPIO_LED_STATUS     GPIO_NUM_12
#define GPIO_BT_STATUS      GPIO_NUM_13
#elif RECEIVER_PPM
#define GPIO_LED_STATUS     4
#define GPIO_PPM_OUT        12
#define GPIO_BIND_BUTTON    13
#elif RX_SE  // receiver SE pin sets
#define GPIO_PPM_OUT  GPIO_NUM_10
#define GPIO_LED_STATUS     GPIO_NUM_4

#endif



/**
 * @brief receiver main fucntion
 */
void receiver_start(void);

#endif