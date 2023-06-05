#include "imu.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "io.h"
#include "ppm.h"
#include "Fusion/Fusion.h"
#include "trackersettings.h"

//------------------------------------------------------------------------------
// Defines
#define DT_IMU DT_NODELABEL(icm42688)

//------------------------------------------------------------------------------
// Macro Modules
LOG_MODULE_REGISTER(imuLog, LOG_LEVEL_DBG);
K_MUTEX_DEFINE(imu_mutex);

//------------------------------------------------------------------------------
// Private Functions Declarations
float normalize(const float value, const float start, const float end);

//------------------------------------------------------------------------------
// Private Values

static FusionVector racc = {0}; // Raw values in m/s^2
static FusionVector rgyr = {0}; // Raw values in rad/s
static FusionVector rmag = {0}; // Raw values in guss
static FusionVector acc = {0};  // in m/s^2
static FusionVector gyr = {0};  // in rad/s
static FusionVector mag = {0};  // in guss
static float tilt = 0, roll = 0, pan = 0;
static float rolloffset = 0, panoffset = 0, tiltoffset = 0; // Center offset, used when pressed center button

static uint16_t channel_data[MAX_CHANNELS] = {PPM_CENTER}; // Range 0-2500

// static float accxoff = 0, accyoff = 0, acczoff = 0;
// static float gyrxoff = 0, gyryoff = 0, gyrzoff = 0;

/**
 * @brief IMU thread runs when the signal is set.
 */
static struct k_poll_signal imuThreadRunSignal = K_POLL_SIGNAL_INITIALIZER(imuThreadRunSignal);
struct k_poll_event imuRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &imuThreadRunSignal),
};

/**
 * @brief Calculate thread runs when the signal is set.
 */
static struct k_poll_signal calculateThreadRunSignal =
    K_POLL_SIGNAL_INITIALIZER(calculateThreadRunSignal);
struct k_poll_event calculateRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &calculateThreadRunSignal),
};

//--------------------Function Defines--------------------

/**
 * @brief Initialises the IMU device.
 */
int imu_Init(void)
{
    const struct device *imu_Dev = DEVICE_DT_GET(DT_IMU);
    if (!device_is_ready(imu_Dev))
    {
        LOG_ERR("Device %s is not ready\r\n", imu_Dev->name);
        return -ENODEV;
    }
    else
    {
        // Start reading the IMU sensors + fusion algorithm
        k_poll_signal_raise(&imuThreadRunSignal, 1);
    }

    // Set IMU Offset. For Test
    // gyrxoff = 0.004608 - 0.001077;
    // gyryoff = -0.0253 + 0.00173;
    // gyrzoff = -0.002058 + 0.001139;

    // accxoff = 0;
    // accyoff = 0;
    // acczoff = 0;

    return 0;
}

/**
 * @brief IMU Reading Channel Thread.
 */
void imu_Thread(void)
{
    int64_t usImuElapse = 0;
    while (1)
    {
        // Only run when imuThreadRunSignal is raised
        k_poll(imuRunEvents, 1, K_FOREVER);

        usImuElapse = micros64(); // Timestamp record
        // printf("[%s] imu0\r\n", now_str()); // test
        const struct device *imu_Dev = DEVICE_DT_GET(DT_IMU);

        k_mutex_lock(&imu_mutex, K_FOREVER);
        if (!sensor_sample_fetch(imu_Dev))
        {
            struct sensor_value tmp[3];
            uint8_t res = 0;

            // 1) Read Raw IMU Data
            // -- Accelerometer
            res = sensor_channel_get(imu_Dev, SENSOR_CHAN_ACCEL_XYZ, &tmp);
            if (!res)
            {
                racc.axis.x = (float)sensor_value_to_double(&tmp[0]);
                racc.axis.y = (float)sensor_value_to_double(&tmp[1]);
                racc.axis.z = (float)sensor_value_to_double(&tmp[2]);

                memcpy(acc.array, racc.array, sizeof(acc));

                // acc.axis.x = racc.axis.x - accxoff;
                // acc.axis.y = racc.axis.y - accyoff;
                // acc.axis.z = racc.axis.z - acczoff;
            }
            else
                LOG_ERR("Get accelerometer data failed!");

            // -- Gyrometer
            res = sensor_channel_get(imu_Dev, SENSOR_CHAN_GYRO_XYZ, &tmp);
            if (!res)
            {
                rgyr.axis.x = (float)sensor_value_to_double(&tmp[0]);
                rgyr.axis.y = (float)sensor_value_to_double(&tmp[1]);
                rgyr.axis.z = (float)sensor_value_to_double(&tmp[2]);

                memcpy(gyr.array, rgyr.array, sizeof(gyr));

                // gyr.axis.x = rgyr.axis.x - gyrxoff;
                // gyr.axis.y = rgyr.axis.y - gyryoff;
                // gyr.axis.z = rgyr.axis.z - gyrzoff;
            }
            else
                LOG_ERR("Get gyrometer data failed!");

            // -- Magnetometer
            if (isUsingMagn())
            {
                res = sensor_channel_get(imu_Dev, SENSOR_CHAN_MAGN_XYZ, &tmp);
                if (!res)
                {
                    rmag.axis.x = (float)sensor_value_to_double(&tmp[0]);
                    rmag.axis.y = (float)sensor_value_to_double(&tmp[1]);
                    rmag.axis.z = (float)sensor_value_to_double(&tmp[2]);
                }
                else
                    LOG_ERR("Get gyrometer data failed!");
            }
            else
            {
                mag.axis.x = 0;
                mag.axis.y = 0;
                mag.axis.z = 0;
            }

            // 3) Apply Rotation
        }
        else
        {
            LOG_ERR("Get IMU data failed!");
        }
        k_mutex_unlock(&imu_mutex);

        // printf("\r\n\r\n[%s]\r\n", now_str());                               // test
        // printf("rgyr: %f,%f,%f\r\n", rgyr.axis.x, rgyr.axis.y, rgyr.axis.z); // test
        // printf("racc: %f,%f,%f\r\n", racc.axis.x, racc.axis.y, racc.axis.z); // test
        // printf("gyr0: %f,%f,%f\r\n", gyr.axis.x, gyr.axis.y, gyr.axis.z);    // test
        // printf("acc0: %f,%f,%f\r\n", acc.axis.x, acc.axis.y, acc.axis.z);    // test

        // // Start doing the other calculations
        // printf("[%s] imu1\r\n", now_str()); // test
        k_poll_signal_raise(&calculateThreadRunSignal, 1);

        // Adjust sleep for a more accurate period
        usImuElapse = micros64() - usImuElapse;
        // printf("[%s] %lld\r\n", now_str(), usImuElapse); // test
        // Took a long time. Will crash if sleep is too short
        if (IMU_PERIOD - usImuElapse < IMU_PERIOD * 0.5)
        {
            k_usleep(IMU_PERIOD);
        }
        else
        {
            k_usleep(IMU_PERIOD - usImuElapse);
        }
    }
}

/**
 * @brief Calculations and Main Channel Thread.
 */
void calculate_Thread(void)
{
    // Define calibration (replace with actual calibration data if available)
    const FusionMatrix gyroscopeMisalignment = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    const FusionVector gyroscopeSensitivity = {1.0f, 1.0f, 1.0f};
    const FusionMatrix accelerometerMisalignment = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    const FusionVector accelerometerSensitivity = {1.0f, 1.0f, 1.0f};
    const FusionMatrix softIronMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    const FusionVector hardIronOffset = {0.0f, 0.0f, 0.0f};

    FusionVector gyroscopeOffset;
    FusionVector accelerometerOffset;

    memcpy(accelerometerOffset.array, getAccOffset(), sizeof(accelerometerOffset));
    memcpy(gyroscopeOffset.array, getGyrOffset(), sizeof(gyroscopeOffset));

    // printf("accelerometerOffset: %f %f %f\r\n", accelerometerOffset.array[0], accelerometerOffset.array[1], accelerometerOffset.array[2]);
    // printf("gyroscopeOffset: %f %f %f\r\n", gyroscopeOffset.array[0], gyroscopeOffset.array[1], gyroscopeOffset.array[2]);

    // Initialise algorithms
    FusionOffset offset;
    FusionAhrs ahrs;

    FusionOffsetInitialise(&offset, SAMPLE_RATE);
    FusionAhrsInitialise(&ahrs);

    // Set AHRS algorithm settings
    const FusionAhrsSettings settings = {
        .convention = FusionConventionNwu,
        .gain = 0.5f,
        .accelerationRejection = 10.0f,
        .magneticRejection = 20.0f,
        .rejectionTimeout = 5 * SAMPLE_RATE, /* 5 seconds */
    };
    FusionAhrsSetSettings(&ahrs, &settings);

    // This loop should repeat each time new gyroscope data is available
    while (true)
    {
        // printf("[%s] cal0\r\n", now_str()); // test

        k_poll(calculateRunEvents, 1, K_FOREVER);

        // Acquire latest sensor data
        const int64_t timestamp = micros64();            // replace this with actual gyroscope timestamp
        FusionVector gyroscope = {0.0f, 0.0f, 0.0f};     // replace this with actual gyroscope data in degrees/s
        FusionVector accelerometer = {0.0f, 0.0f, 1.0f}; // replace this with actual accelerometer data in g
        FusionVector magnetometer = {1.0f, 0.0f, 0.0f};  // replace this with actual magnetometer data in arbitrary units

        // Use a mutex so sensor data can't be updated part way
        k_mutex_lock(&imu_mutex, K_FOREVER);
        gyroscope.axis.x = rad_to_degrees(gyr.axis.x);
        gyroscope.axis.y = rad_to_degrees(gyr.axis.y);
        gyroscope.axis.z = rad_to_degrees(gyr.axis.z);

        accelerometer.axis.x = ms2_to_g(acc.axis.x);
        accelerometer.axis.y = ms2_to_g(acc.axis.y);
        accelerometer.axis.z = ms2_to_g(acc.axis.z);
        // printf("%f,%f,%f\r\n", gyroscope.axis.x, gyroscope.axis.y, gyroscope.axis.z); // test
        // printf("%f,%f,%f\r\n", gyroscope.axis.x-gyroscopeOffset.axis.x, gyroscope.axis.y-gyroscopeOffset.axis.y, gyroscope.axis.z-gyroscopeOffset.axis.z); // test

        if (isUsingMagn())
        {
            magnetometer.axis.x = mag.axis.x;
            magnetometer.axis.y = mag.axis.y;
            magnetometer.axis.z = mag.axis.z;

            magnetometer = FusionCalibrationMagnetic(magnetometer, softIronMatrix, hardIronOffset);
        }
        k_mutex_unlock(&imu_mutex);

        // Apply calibration
        gyroscope = FusionCalibrationInertial(gyroscope, gyroscopeMisalignment, gyroscopeSensitivity, gyroscopeOffset);
        accelerometer = FusionCalibrationInertial(accelerometer, accelerometerMisalignment, accelerometerSensitivity, accelerometerOffset);

        // Update gyroscope offset correction algorithm
        gyroscope = FusionOffsetUpdate(&offset, gyroscope);

        // Calculate delta time (in seconds) to account for gyroscope sample clock error
        static int64_t previousTimestamp;
        const float deltaTime = (float)(timestamp - previousTimestamp) / (float)USEC_PER_SEC;
        previousTimestamp = timestamp;

        // Update gyroscope AHRS algorithm
        if (isUsingMagn())
            FusionAhrsUpdate(&ahrs, gyroscope, accelerometer, magnetometer, deltaTime);
        else
            FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, deltaTime);

        // Convert pitch/roll/yaw to roll/tilt/pan
        const FusionEuler euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
        tilt = euler.angle.roll;
        roll = euler.angle.pitch;
        pan = euler.angle.yaw;

        // printf("Roll %0.1f, Pitch %0.1f, Yaw %0.1f\r\n"
        //        "Ax %f, Ay %f, Az %f\r\n"
        //        "Gx %f, Gy %f, Gz %f\r\n"
        //        "Mx %f, My %f, Mz %f\r\n",
        //        euler.angle.roll, euler.angle.pitch, euler.angle.yaw,
        //        accelerometer.axis.x, accelerometer.axis.y, accelerometer.axis.z,
        //        gyroscope.axis.x, gyroscope.axis.y, gyroscope.axis.z,
        //        magnetometer.axis.x, magnetometer.axis.y, magnetometer.axis.z);

        // printf("[%s]:\n"
        //        "accel %f %f %f m/s/s\n"
        //        "gyro  %f %f %f rad/s\n"
        //        "magn  %f %f %f gauss\r\n",
        //        now_str(),
        //        accx, accy, accz,
        //        gyrx, gyry, gyrz,
        //        magx, magy, magz);

        // Zero button was pressed, adjust all values to zero
        bool butdnw = false;
        if (wasButtonPressed())
        {
            rolloffset = roll;
            panoffset = pan;
            tiltoffset = tilt;
            butdnw = true;
        }

        // Tilt output
        float tiltout =
            (tilt - tiltoffset) * getTiltGain() * (isTiltReversed() ? -1.0 : 1.0);
        uint16_t tiltout_ui = tiltout + getTiltCnt();                  // Apply Center Offset
        tiltout_ui = MAX(MIN(tiltout_ui, getTiltMax()), getTiltMin()); // Limit Output

        // Roll output
        float rollout =
            (roll - rolloffset) * getRollGain() * (isRollReversed() ? -1.0 : 1.0);
        uint16_t rollout_ui = rollout + getRollCnt();                  // Apply Center Offset
        rollout_ui = MAX(MIN(rollout_ui, getRollMax()), getRollMin()); // Limit Output

        // Pan output, Normalize to +/- 180 Degrees
        float panout = normalize((pan - panoffset), -180, 180) * getPanGain() *
                       (isPanReversed() ? -1.0 : 1.0);
        uint16_t panout_ui = panout + getPanCnt();                 // Apply Center Offset
        panout_ui = MAX(MIN(panout_ui, getPanMax()), getPanMin()); // Limit Output

        // // Reset on tilt
        // static bool doresetontilt = false;
        // if (trkset.getRstOnTlt())
        // {
        //     static bool tiltpeak = false;
        //     static float resettime = 0.0f;
        //     enum
        //     {
        //         HITNONE,
        //         HITMIN,
        //         HITMAX,
        //     };
        //     static int minmax = HITNONE;
        //     if (rollout_ui == trkset.getRll_Max())
        //     {
        //         if (tiltpeak == false && minmax == HITNONE)
        //         {
        //             tiltpeak = true;
        //             minmax = HITMAX;
        //         }
        //         else if (minmax == HITMIN)
        //         {
        //             minmax = HITNONE;
        //             tiltpeak = false;
        //             doresetontilt = true;
        //         }
        //     }
        //     else if (rollout_ui == trkset.getRll_Min())
        //     {
        //         if (tiltpeak == false && minmax == HITNONE)
        //         {
        //             tiltpeak = true;
        //             minmax = HITMIN;
        //         }
        //         else if (minmax == HITMAX)
        //         {
        //             minmax = HITNONE;
        //             tiltpeak = false;
        //             doresetontilt = true;
        //         }
        //     }

        //     // If hit a max/min wait an amount of time and reset it
        //     if (tiltpeak == true)
        //     {
        //         resettime += (float)CALCULATE_PERIOD / 1000000.0;
        //         if (resettime > TrackerSettings::RESET_ON_TILT_TIME)
        //         {
        //             tiltpeak = false;
        //             minmax = HITNONE;
        //             resettime = 0;
        //         }
        //     }
        // }

        // // Do the actual reset after a delay
        // static float timetoreset = 0;
        // if (doresetontilt)
        // {
        //     if (timetoreset > TrackerSettings::RESET_ON_TILT_AFTER)
        //     {
        //         doresetontilt = false;
        //         timetoreset = 0;
        //         pressButton();
        //     }
        //     timetoreset += (float)CALCULATE_PERIOD / 1000000.0;
        // }

        // Build PPM channel data
        // Reset all channels to center
        for (uint8_t i = 0; i < (sizeof(channel_data) / sizeof(channel_data[0])); i++)
            channel_data[i] = PPM_CENTER;

        // Set Tilt/Roll/Pan Channel Values
        uint8_t tltch = getTiltChl();
        uint8_t rllch = getRollChl();
        uint8_t panch = getPanChl();
        if (tltch > 0)
            channel_data[tltch - 1] = isTiltEn() == true ? tiltout_ui : getTiltCnt();
        if (rllch > 0)
            channel_data[rllch - 1] = isRollEn() == true ? rollout_ui : getRollCnt();
        if (panch > 0)
            channel_data[panch - 1] = isPanEn() == true ? panout_ui : getPanCnt();

        // 10) Set the PPM Outputs
        // printf("%f,%f,%f\r\n", tiltout, rollout, panout); // test
        // printf("%d,%d,%d\r\n", tiltout_ui, rollout_ui, panout_ui); // test
        for (uint8_t i = 0; i < PpmOut_getChnCount(); i++)
        {
            uint16_t ppmout = channel_data[i];
            if (ppmout == 0)
                ppmout = PPM_CENTER;
            // printf("%d  ", ppmout); //test
            PpmOut_setChannel(i, ppmout);
        }
        buildChannels();

        // printf("\r\n"); //test
        int elipsed = micros64() - timestamp;
        // printf("[%d]:\r\n", elipsed);

        // printf("%0.1f,%0.1f,%0.1f\r\n", tilt - tiltoffset,
        //        roll - rolloffset,
        //        pan - panoffset); // test
        // printf("%f,%f,%f\r\n", racc.axis.x, racc.axis.y, racc.axis.z); // test
        // printf("%f,%f,%f\r\n", gyr.axis.x, gyr.axis.y, gyr.axis.z); // test
        // printf("%f,%f,%f\r\n", gyroscope.axis.x, gyroscope.axis.y, gyroscope.axis.z); // test
        // printf("%f,%f,%f\r\n", accelerometer.axis.x, accelerometer.axis.y, accelerometer.axis.z); // test

        // printPPMdata(); // test

        k_poll_signal_reset(&calculateThreadRunSignal);
        // printf("[%s] cal1\r\n", now_str()); // test

        // // Adjust sleep for a more accurate period
        // usImuElapse = micros64() - usImuElapse;
        // // Took a long time. Will crash if sleep is too short
        // if (CALCULATE_PERIOD - usImuElapse < CALCULATE_PERIOD * 0.7)
        // {
        //     k_usleep(CALCULATE_PERIOD);
        // }
        // else
        // {
        //     k_usleep(CALCULATE_PERIOD - usImuElapse);
        // }
    }
}

// FROM https://stackoverflow.com/questions/1628386/normalise-orientation-between-0-and-360
// Normalizes any number to an arbitrary range
// by assuming the range wraps around when going below min or above max
float normalize(const float value, const float start, const float end)
{
    const float width = end - start;         //
    const float offsetValue = value - start; // value relative to 0

    return (offsetValue - (floor(offsetValue / width) * width)) + start;
    // + start to reset back to start of original range
}

void getChannelData(uint16_t *buffer)
{
    memcpy(buffer, channel_data, sizeof(channel_data));
}

const char *now_str(void)
{
    static char buf[24]; /* ...HH:MM:SS.MMM,UUU */
    uint64_t now = micros64();
    uint16_t us = now % USEC_PER_MSEC;
    uint16_t ms;
    uint8_t s;
    uint8_t min;
    uint32_t h;

    now /= USEC_PER_MSEC;
    ms = now % MSEC_PER_SEC;
    now /= MSEC_PER_SEC;
    s = now % 60U;
    now /= 60U;
    min = now % 60U;
    now /= 60U;
    h = now;

    snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u,%03u",
             h, min, s, ms, us);
    return buf;
}