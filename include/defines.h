#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"

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


// Thread stack size
#define IO_THREAD_STACK_SIZE    configMINIMAL_STACK_SIZE*1
#define IMU_THREAD_STACK_SIZE   configMINIMAL_STACK_SIZE*5  //thread stack size
#define CAL_THREAD_STACK_SIZE   configMINIMAL_STACK_SIZE*20  //thread stack size
#define BT_THREAD_STACK_SIZE    configMINIMAL_STACK_SIZE*10  //thread stack size

// Thread period in milisecond
#define IMU_THREAD_PERIOD   10
#define CAL_THREAD_PERIOD   10
#define BT_THREAD_PERIOD    8

#define SAMPLE_RATE (int)(1000 / IMU_THREAD_PERIOD) // IMU sample rate, replace this with actual sample rate(Hz)

#define millis64()  esp_log_timestamp()
#define micros64()  esp_timer_get_time()

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))