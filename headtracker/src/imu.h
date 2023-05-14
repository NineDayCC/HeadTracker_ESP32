#ifndef IMU_H
#define IMU_H

//------------------------------------------------------------------------------
// Includes

#include <math.h> // M_PI, sqrtf, atan2f, asinf
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"

//------------------------------------------------------------------------------
// Definitions


/**
 * @brief Converting radians to degrees.
 * @param rad Value in radians.
 * @return Value in degrees.
 */
inline float rad_to_degrees(float rad)
{
    return (rad * (180.0f / (float) M_PI));
}

/**
 * @brief Converting m/s^2 to g.
 * @param rad Value in m/s^2.
 * @return Value in g.
 */
inline float ms2_to_g(float ms2)
{
    return (ms2 / (float) M_G);
}
int imu_Init(void);
void imu_Thread(void);
void calculate_Thread(void);


#endif