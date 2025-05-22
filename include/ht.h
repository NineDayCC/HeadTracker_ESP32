#pragma once
#ifdef HEADTRAKCER

#include "defines.h"

// Pins set

#ifdef HT_NANO_V2  // Headtracker Lite pin sets
#define PIN_NUM_MISO        GPIO_NUM_6
#define PIN_NUM_MOSI        GPIO_NUM_7
#define PIN_NUM_CLK         GPIO_NUM_4
#define PIN_NUM_CS          GPIO_NUM_5
#define GPIO_CENTER_BUTTON  GPIO_NUM_1
#define GPIO_LED_STATUS     GPIO_NUM_3
#define GPIO_OTA_BUTTON     GPIO_NUM_9
#define GPIO_BUZZER         GPIO_NUM_10
#elif HT_NANO  // Headtracker Nano pin sets
#define PIN_NUM_MISO        GPIO_NUM_13
#define PIN_NUM_MOSI        GPIO_NUM_15
#define PIN_NUM_CLK         GPIO_NUM_2
#define PIN_NUM_CS          GPIO_NUM_0
#define GPIO_CENTER_BUTTON  GPIO_NUM_27
#define GPIO_LED_STATUS     GPIO_NUM_14
#define GPIO_OTA_BUTTON     GPIO_NUM_20
#define GPIO_BUZZER         GPIO_NUM_25
#elif HT_SE  // Headtracker Nano pin sets
#define PIN_NUM_MISO        GPIO_NUM_6
#define PIN_NUM_MOSI        GPIO_NUM_7
#define PIN_NUM_CLK         GPIO_NUM_5
#define PIN_NUM_CS          GPIO_NUM_4
#define GPIO_CENTER_BUTTON  GPIO_NUM_3
#define GPIO_LED_STATUS     GPIO_NUM_1
#define GPIO_BUZZER         GPIO_NUM_10

#endif

// IMU speed
#define IMU_SPI_SPEED_HZ    (8 * 1000 * 1000)  //Clock out at 8 MHz



/**
 * @brief headtracker main fucntion
 */
void headtracker_start(void);

#endif