#pragma once
#ifdef HEADTRAKCER

#include "ht.h"

#define PIN_NUM_MISO_SET    PIN_NUM_MISO
#define PIN_NUM_MOSI_SET    PIN_NUM_MOSI
#define PIN_NUM_CLK_SET     PIN_NUM_CLK
#define PIN_NUM_CS_SET      PIN_NUM_CS
#define IMU_SPI_SPEED_SET   IMU_SPI_SPEED_HZ

#define IMU_THREAD_PRIORITY_SET      IMU_THREAD_PRIORITY  //thread priority
#define IMU_THREAD_STACK_SIZE_SET    IMU_THREAD_STACK_SIZE  //thread stack size
#define CAL_THREAD_PRIORITY_SET      CAL_THREAD_PRIORITY  //thread priority
#define CAL_THREAD_STACK_SIZE_SET    CAL_THREAD_STACK_SIZE  //thread stack size

typedef union
{
    float array[3];
    struct
    {
        float roll;
        float tilt;
        float pan;
    } axis;
} EularVactor;

typedef struct
{
    uint8_t hold : 1; // hold current output position
    uint8_t reserved : 7;
    EularVactor eularHold;  //output eular when holding
} ImuStatus;

//------------------------------------------------------------------------------
// Definitions

/**
 * @brief Read Channel data(pwm values).
 * @param buffer destiny pointer, must be no smaller then sizeof(channel_data).
 */
void getChannelData(uint16_t *buffer);
float normalize(const float value, const float start, const float end);

int imu_Init(void);
void imu_Thread(void *pvParameters);
void calculate_Thread(void *pvParameters);

#endif