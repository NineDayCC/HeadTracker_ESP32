#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#ifndef FRAMEWORK_ARDUINO
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"
#endif

// Thread Priority Definitions
#define PRIORITY_LOW 5
#define PRIORITY_MED 10
#define PRIORITY_HIGH 15
#define PRIORITY_RT 20

// Thread priority
#define IO_THREAD_PRIORITY  PRIORITY_LOW
#define IMU_THREAD_PRIORITY PRIORITY_MED
#define CAL_THREAD_PRIORITY PRIORITY_HIGH
#define BT_THREAD_PRIORITY  PRIORITY_RT
#define MODE_THREAD_PRIORITY PRIORITY_HIGH
#define ESPNOW_THREAD_PRIORITY PRIORITY_HIGH


// Thread stack size
#define IO_THREAD_STACK_SIZE    configMINIMAL_STACK_SIZE*1
#define IMU_THREAD_STACK_SIZE   configMINIMAL_STACK_SIZE*5  //thread stack size
#define CAL_THREAD_STACK_SIZE   configMINIMAL_STACK_SIZE*20  //thread stack size
#define BT_THREAD_STACK_SIZE    configMINIMAL_STACK_SIZE*10  //thread stack size
#define MODE_THREAD_STACK_SIZE  configMINIMAL_STACK_SIZE*2
#define ESPNOW_THREAD_STACK_SIZE configMINIMAL_STACK_SIZE*5

// Thread period in milisecond
#define IMU_THREAD_PERIOD   10
#define CAL_THREAD_PERIOD   10
#define BT_THREAD_PERIOD    8
#define ESPNOW_SEND_PERIOD  10
#define MODE_THREAD_PERIOD  50

#define SAMPLE_RATE (int)(1000 / IMU_THREAD_PERIOD) // IMU sample rate, replace this with actual sample rate(Hz)

#ifndef FRAMEWORK_ARDUINO
#define millis64()  esp_log_timestamp()
#define micros64()  esp_timer_get_time()
#endif

#ifndef MIN
#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#endif
#ifndef MAX
#define MAX(i, j) (((i) > (j)) ? (i) : (j))
#endif
