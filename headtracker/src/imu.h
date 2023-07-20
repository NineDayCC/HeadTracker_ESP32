#ifndef IMU_H
#define IMU_H

//------------------------------------------------------------------------------
// Includes

#include <zephyr/kernel.h>
#include <math.h> // M_PI, sqrtf, atan2f, asinf

#include "defines.h"

//------------------------------------------------------------------------------
// Type
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

int imu_Init(void);
void imu_Thread(void);
void calculate_Thread(void);

const char *now_str(void);

#endif