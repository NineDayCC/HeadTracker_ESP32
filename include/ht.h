#pragma once

#include "defines.h"

// Pins set

#ifdef HT_LITE  // Headtracker Lite pin sets
#define PIN_NUM_MISO        GPIO_NUM_4
#define PIN_NUM_MOSI        GPIO_NUM_5
#define PIN_NUM_CLK         GPIO_NUM_6
#define PIN_NUM_CS          GPIO_NUM_7
#define GPIO_CENTER_BUTTON  GPIO_NUM_9
#define GPIO_PPM_OUT        GPIO_NUM_10

#endif

// IMU speed
#define IMU_SPI_SPEED_HZ    (8 * 1000 * 1000)  //Clock out at 8 MHz



/**
 * @brief headtracker main fucntion
 */
void headtracker_start(void);