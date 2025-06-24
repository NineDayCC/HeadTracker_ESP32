#ifdef HEADTRACKER

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "defines.h"

#include "io.h"
#include "bt.h"
#include "app_espnow.h"
#include "imu.h"
#include "ppm.h"
#include "Fusion.h"
#include "icm42688.h"
#include "trackersettings.h"

//------------------------------------------------------------------------------
// Defines
#define IMU_HOST SPI2_HOST // only spi2 is suitable on esp32_c3
#define IMU_HOLD 1
#define IMU_RECOVER 0

//------------------------------------------------------------------------------
// Values

static const char *IMU_TAG = "IMU";
#ifdef USE_ICM42688
static spi_device_handle_t imu_dev;
#else
static spi_device_handle_t imu_dev;
#endif

#define GOGGLE_G2 0
#define GOGGLE_N3 1
#define IMU_GOGGLE_PRESET GOGGLE_G2
// Install angle rotation
#if IMU_GOGGLE_PRESET == GOGGLE_G2
const float R[3][3] = {
    {0.9816272, -0.1736482, 0.0868241},
    {0.1736482, 0.9848078, 0.0},
    {-0.0871557, 0.0, 0.9961947}};
#elif IMU_GOGGLE_PRESET == GOGGLE_N3
const float R[3][3] = {
    {0, 0, -1},
    {1, 0, 0},
    {0, -1, 0}};

#endif

static TaskHandle_t imuTaskHandle;
static TaskHandle_t calculateTaskHandle;

static FusionVector racc = {0}; // Raw values in g
static FusionVector rgyr = {0}; // Raw values in d/s
static FusionVector rmag = {0}; // Raw values in guss
static FusionVector acc = {0};  // in g
static FusionVector gyr = {0};  // in d/s
static FusionVector mag = {0};  // in guss
static float tilt = 0, roll = 0, pan = 0;
static float rolloffset = 0, panoffset = 0, tiltoffset = 0; // Center offset, used when pressed center button

static uint16_t channel_data[MAX_CHANNELS] = {PPM_CENTER}; // Range 0-2500

static ImuStatus imuStatus = {
    .hold = 0,
    .reserved = 0,
    .eularHold = {{0, 0, 0}},
};

static SemaphoreHandle_t calculateThreadRunSignal = NULL;
//------------------------------------------------------------------------------
//--------------------Function Defines--------------------

/**
 * @brief Initialises the IMU device.
 */
int imu_Init(void)
{
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO_SET,
        .mosi_io_num = PIN_NUM_MOSI_SET,
        .sclk_io_num = PIN_NUM_CLK_SET,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1024, // must be bigger than (SPI_RX_MAX_SIZE + 1), SPI_RX_MAX_SIZE is defined in icm42688.c
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = IMU_SPI_SPEED_SET, // icm42688 support 24MHz Maximum
        .mode = 0,                           // SPI mode 0
        .spics_io_num = PIN_NUM_CS_SET,      // CS pin
        .queue_size = 1,                     // We want to be able to queue 1 transactions at a time
    };
    // Initialize the SPI bus
    ret = spi_bus_initialize(IMU_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    // Attach the IMU to the SPI bus
    ret = spi_bus_add_device(IMU_HOST, &devcfg, &imu_dev);
    ESP_ERROR_CHECK(ret);
    // Initialize the imu
    icm42688_Init(imu_dev);

    // create semaphore
    calculateThreadRunSignal = xSemaphoreCreateBinary();
    if (calculateThreadRunSignal == NULL)
    {
        ESP_LOGW(IMU_TAG, "calculateThreadRunSignal create FAILED");
        return ESP_FAIL;
    }

    // create task thread
    ESP_LOGI(IMU_TAG, "xTaskCreate");
#ifdef HT_NANO
    xTaskCreatePinnedToCore(imu_Thread, "imu_Thread", IMU_THREAD_STACK_SIZE_SET, NULL, IMU_THREAD_PRIORITY_SET, NULL, 1);             // run on core1
    xTaskCreatePinnedToCore(calculate_Thread, "calculate_Thread", CAL_THREAD_STACK_SIZE_SET, NULL, CAL_THREAD_PRIORITY_SET, NULL, 1); // run on core1
#elif defined HT_NANO_V2 || defined HT_SE
    xTaskCreate(imu_Thread, "imu_Thread", IMU_THREAD_STACK_SIZE_SET, NULL, IMU_THREAD_PRIORITY_SET, &imuTaskHandle);             // run on core0
    xTaskCreate(calculate_Thread, "calculate_Thread", CAL_THREAD_STACK_SIZE_SET, NULL, CAL_THREAD_PRIORITY_SET, &calculateTaskHandle); // run on core0
#endif
    // for (;;)
    // {
    //     // acc data size(2 bytes * 3) + gyro data size(2 bytes * 3)
    //     uint8_t readings[12] = {0};
    //     float test_data[6] = {0};
    //     // 1) Read Raw IMU Data
    //     if (!icm42688_AccGyr_fetch(imu_dev, readings, sizeof(readings)))
    //     {
    //         icm42688_acc_get(&readings[0], test_data);
    //         icm42688_gyr_get(&readings[6], &test_data[3]);
    //         ESP_LOGI(IMU_TAG,"%x,%x,%x,%x,%x,%x",readings[0],readings[1],readings[2],readings[3],readings[4],readings[5]);
    //         ESP_LOGI(IMU_TAG,"%f,%f,%f",test_data[0],test_data[1],test_data[2]);
    //     }
    //     // int64_t b = micros64();
    //     // int64_t a = millis64();
    //     // printf("%lld ms,%lld us\n",a,b);
    //     vTaskDelay(pdMS_TO_TICKS(100));
    // }
    return ret;
}

/**
 * @brief IMU Reading Channel Thread.
 */
void imu_Thread(void *pvParameters)
{
    TickType_t xLastWakeTime;

    // Initialise the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {

#ifdef USE_ICM42688
        // ICM42688--------------------------

        // acc data size(2*3) + gyro data size(2*3)
        uint8_t readings[12];
        // 1) Read Raw IMU Data
        if (!icm42688_AccGyr_fetch(imu_dev, readings, sizeof(readings)))
        {
            icm42688_acc_get(&readings[0], racc.array);
            icm42688_gyr_get(&readings[6], rgyr.array);

            memcpy(acc.array, racc.array, sizeof(acc));
            memcpy(gyr.array, rgyr.array, sizeof(gyr));
        }
        else
        {
            ESP_LOGW(IMU_TAG, "Get IMU data failed!");
        }
        // ICM42688--------------------------
#endif

        // printf("%f,%f,%f\n", rgyr.axis.x, rgyr.axis.y, rgyr.axis.z); // test

        // printf("\n\n[%ld]\n", xLastWakeTime);                           // test
        // printf("rgyr: %f,%f,%f\n", rgyr.axis.x, rgyr.axis.y, rgyr.axis.z); // test
        // printf("racc: %f,%f,%f\n", racc.axis.x, racc.axis.y, racc.axis.z); // test
        // printf("gyr0: %f,%f,%f\n", gyr.axis.x, gyr.axis.y, gyr.axis.z);    // test
        // printf("acc0: %f,%f,%f\n", acc.axis.x, acc.axis.y, acc.axis.z);    // test

        // Start doing the calculations
        xSemaphoreGive(calculateThreadRunSignal);

        xTaskDelayUntil(&xLastWakeTime, IMU_THREAD_PERIOD);
    }
}

/**
 * @brief Calculations and Main Channel Thread.
 */
void calculate_Thread(void *pvParameters)
{
    // Define calibration (replace with actual calibration data if available)
    const FusionMatrix gyroscopeMisalignment = {{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    const FusionVector gyroscopeSensitivity = {{1.0f, 1.0f, 1.0f}};
    const FusionMatrix accelerometerMisalignment = {{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    const FusionVector accelerometerSensitivity = {{1.0f, 1.0f, 1.0f}};
    const FusionMatrix softIronMatrix = {{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
    const FusionVector hardIronOffset = {{0.0f, 0.0f, 0.0f}};

    FusionVector gyroscopeOffset;
    FusionVector accelerometerOffset;

    for (uint8_t i = 0; i < 3; i++)
    {
        accelerometerOffset.array[i] = getAccOffset(i);
        gyroscopeOffset.array[i] = getGyrOffset(i);
    }

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
        .gyroscopeRange = 1000.0f, /* replace this with actual gyroscope range in degrees/s */
        .accelerationRejection = 10.0f,
        .magneticRejection = 10.0f,
        .recoveryTriggerPeriod = 5 * SAMPLE_RATE, /* 5 seconds */
    };
    FusionAhrsSetSettings(&ahrs, &settings);

    // This loop should repeat each time new gyroscope data is available
    for (;;)
    {
        xSemaphoreTake(calculateThreadRunSignal, portMAX_DELAY);

        // Acquire latest sensor data
        const int64_t timestamp = micros64();                    // replace this with actual gyroscope timestamp
        FusionVector gyroscope = {{{0.0f}, {0.0f}, {0.0f}}};     // replace this with actual gyroscope data in degrees/s
        FusionVector accelerometer = {{{0.0f}, {0.0f}, {1.0f}}}; // replace this with actual accelerometer data in g
        FusionVector magnetometer = {{{1.0f}, {0.0f}, {0.0f}}};  // replace this with actual magnetometer data in arbitrary units

        // Rotate the imu data
        accelerometer.axis.x = (R[0][0] * acc.axis.x + R[0][1] * acc.axis.y + R[0][2] * acc.axis.z);
        accelerometer.axis.y = (R[1][0] * acc.axis.x + R[1][1] * acc.axis.y + R[1][2] * acc.axis.z);
        accelerometer.axis.z = (R[2][0] * acc.axis.x + R[2][1] * acc.axis.y + R[2][2] * acc.axis.z);
        gyroscope.axis.x = (R[0][0] * gyr.axis.x + R[0][1] * gyr.axis.y + R[0][2] * gyr.axis.z);
        gyroscope.axis.y = (R[1][0] * gyr.axis.x + R[1][1] * gyr.axis.y + R[1][2] * gyr.axis.z);
        gyroscope.axis.z = (R[2][0] * gyr.axis.x + R[2][1] * gyr.axis.y + R[2][2] * gyr.axis.z);
        // memcpy(accelerometer.array, acc.array, sizeof(accelerometer));
        // memcpy(gyroscope.array, gyr.array, sizeof(gyroscope));

        if (isUsingMagn())
        {
            memcpy(magnetometer.array, mag.array, sizeof(magnetometer));
            magnetometer = FusionCalibrationMagnetic(magnetometer, softIronMatrix, hardIronOffset);
            magnetometer = FusionCalibrationMagnetic(magnetometer, softIronMatrix, hardIronOffset);
        }

        // Apply calibration
        gyroscope = FusionCalibrationInertial(gyroscope, gyroscopeMisalignment, gyroscopeSensitivity, gyroscopeOffset);
        accelerometer = FusionCalibrationInertial(accelerometer, accelerometerMisalignment, accelerometerSensitivity, accelerometerOffset);

        // Update gyroscope offset correction algorithm
        gyroscope = FusionOffsetUpdate(&offset, gyroscope);

        // Calculate delta time (in seconds) to account for gyroscope sample clock error
        static int64_t previousTimestamp = 0;
        const float deltaTime = (float)(timestamp - previousTimestamp) / (float)1000000.0; // (divide ticks per second)
        previousTimestamp = timestamp;

        // Update gyroscope AHRS algorithm
        if (isUsingMagn())
            FusionAhrsUpdate(&ahrs, gyroscope, accelerometer, magnetometer, deltaTime);
        else
            FusionAhrsUpdateNoMagnetometer(&ahrs, gyroscope, accelerometer, deltaTime);

        // Convert pitch/roll/yaw to roll/tilt/pan
        const FusionEuler euler = FusionQuaternionToEuler(FusionAhrsGetQuaternion(&ahrs));
        roll = euler.angle.roll;
        tilt = euler.angle.pitch;
        pan = euler.angle.yaw;

        // printf("%f,%f,%f\r\n", euler.angle.roll, euler.angle.pitch, euler.angle.yaw); // test

        // printf("Roll %0.1f, Pitch %0.1f, Yaw %0.1f\r\n"
        //        "Ax %f, Ay %f, Az %f\r\n"
        //        "Gx %f, Gy %f, Gz %f\r\n"
        //        "Mx %f, My %f, Mz %f\r\n",
        //        euler.angle.roll, euler.angle.pitch, euler.angle.yaw,
        //        accelerometer.axis.x, accelerometer.axis.y, accelerometer.axis.z,
        //        gyroscope.axis.x, gyroscope.axis.y, gyroscope.axis.z,
        //        magnetometer.axis.x, magnetometer.axis.y, magnetometer.axis.z);

        // Zero button was pressed, adjust all values to zero
        if (isSingleClick())
        {
            rolloffset = roll;
            tiltoffset = tilt;
            panoffset = pan;
            imuStatus.hold = 0; // recover output
        }
        else if (isLongStart())
        {
            switch (imuStatus.hold)
            {
            case IMU_RECOVER:
                imuStatus.hold = IMU_HOLD;
                imuStatus.eularHold.axis.roll = roll;
                imuStatus.eularHold.axis.tilt = tilt;
                imuStatus.eularHold.axis.pan = pan;
                break;
            case IMU_HOLD:
                imuStatus.hold = IMU_RECOVER;
                // Adjust offset to make the output from current position
                rolloffset = roll - (imuStatus.eularHold.axis.roll - rolloffset);
                tiltoffset = tilt - (imuStatus.eularHold.axis.tilt - tiltoffset);
                panoffset = pan - (imuStatus.eularHold.axis.pan - panoffset);
                break;
            default:
                break;
            }
        }

        // Hold the output eular
        if (imuStatus.hold == IMU_HOLD)
        {
            roll = imuStatus.eularHold.axis.roll;
            tilt = imuStatus.eularHold.axis.tilt;
            pan = imuStatus.eularHold.axis.pan;
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

#ifdef HT_LITE
        // 10) Set the PPM Outputs
        for (uint8_t i = 0; i < PpmOut_getChnCount(); i++)
        {
            uint16_t ppmout = channel_data[i];
            if (ppmout == 0)
                ppmout = PPM_CENTER;
            PpmOut_setChannel(i, ppmout);
        }
        buildChannels();

        // // 11) Set the SBUS Outputs
        // sbusBuildChannels(channel_data);

        // // 12) Set the BT Outputs
        buildBtChannels(channel_data, BT_CHANNELS);

#elif defined HT_NANO || defined HT_NANO_V2 || defined HT_SE
        // Send channel data to esp now task.
        espnow_data_prepare(channel_data[tltch - 1], channel_data[rllch - 1], channel_data[panch - 1]);
#endif
        // int elipsed = micros64() - timestamp;
        // printf("[%f]:\n", deltaTime);

        // printf("%f,%f,%f\n", tilt - tiltoffset,
        //        roll - rolloffset,
        //        pan - panoffset); // test

        // FusionAhrsFlags test_flags;
        // test_flags =FusionAhrsGetFlags(&ahrs);
        // printf("%d,%d,%d,%d\n",test_flags.initialising, test_flags.angularRateRecovery, test_flags.accelerationRecovery,test_flags.magneticRecovery);

        // printf("%f,%f,%f\n", racc.axis.x, racc.axis.y, racc.axis.z); // test
        // printf("%f,%f,%f\n", gyr.axis.x, gyr.axis.y, gyr.axis.z); // test
        // printf("%f,%f,%f\n", gyroscope.axis.x, gyroscope.axis.y, gyroscope.axis.z); // test
        // printf("%f,%f,%f\n", accelerometer.axis.x, accelerometer.axis.y, accelerometer.axis.z); // test
        // printf("%f,%f,%f,%f,%f,%f\r\n", gyroscope.array[0], gyroscope.array[1], gyroscope.array[2], accelerometer.array[0], accelerometer.array[1], accelerometer.array[2]); // test

        // printPPMdata(); // test
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

void imu_Deinit(void)
{
    // Delete IMU and calculate tasks
    if (imuTaskHandle != NULL)
    {
        vTaskDelete(imuTaskHandle);
    }

    if (calculateTaskHandle != NULL)
    {
        vTaskDelete(calculateTaskHandle);
    }

    // Free semaphore
    if (calculateThreadRunSignal != NULL)
    {
        vSemaphoreDelete(calculateThreadRunSignal);
        calculateThreadRunSignal = NULL;
    }

    // Remove the IMU device from the SPI bus
    if (imu_dev != NULL)
    {
        spi_bus_remove_device(imu_dev);
        imu_dev = NULL;
    }

    // Free any other dynamically allocated resources if applicable
}



#endif