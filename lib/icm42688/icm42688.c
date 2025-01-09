#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#include "icm42688.h"

//------------------------------------------------------------------------------
// Defines
#define REG_SPI_READ_BIT 0x80
#define SPI_RX_MAX_SIZE 4 * 4  //SPI receive buffer max size. greater than acc(2 bytes * 3) + gyro(2 bytes * 3). 32 bits alignment require
static const char* IMU_TAG = "ICM42688";

//------------------------------------------------------------------------------
// Macro Modules

#define ICM426XX_RA_REG_BANK_SEL 0x76
#define ICM426XX_BANK_SELECT0 0x00
#define ICM426XX_BANK_SELECT1 0x01
#define ICM426XX_BANK_SELECT2 0x02
#define ICM426XX_BANK_SELECT3 0x03
#define ICM426XX_BANK_SELECT4 0x04

#define ICM426XX_RA_PWR_MGMT0 0x4E // User Bank 0
#define ICM426XX_PWR_MGMT0_ACCEL_MODE_LN (3 << 0)
#define ICM426XX_PWR_MGMT0_GYRO_MODE_LN (3 << 2)
#define ICM426XX_PWR_MGMT0_GYRO_ACCEL_MODE_OFF ((0 << 0) | (0 << 2))
#define ICM426XX_PWR_MGMT0_TEMP_DISABLE_OFF (0 << 5)
#define ICM426XX_PWR_MGMT0_TEMP_DISABLE_ON (1 << 5)

#define ICM426XX_RA_GYRO_CONFIG0 0x4F
#define ICM426XX_RA_ACCEL_CONFIG0 0x50

// --- Registers for gyro and acc Anti-Alias Filter ---------
#define ICM426XX_RA_GYRO_CONFIG_STATIC3 0x0C  // User Bank 1
#define ICM426XX_RA_GYRO_CONFIG_STATIC4 0x0D  // User Bank 1
#define ICM426XX_RA_GYRO_CONFIG_STATIC5 0x0E  // User Bank 1
#define ICM426XX_RA_ACCEL_CONFIG_STATIC2 0x03 // User Bank 2
#define ICM426XX_RA_ACCEL_CONFIG_STATIC3 0x04 // User Bank 2
#define ICM426XX_RA_ACCEL_CONFIG_STATIC4 0x05 // User Bank 2
// --- Register & setting for gyro and acc UI Filter --------
#define ICM426XX_RA_GYRO_ACCEL_CONFIG0 0x52 // User Bank 0
#define ICM426XX_ACCEL_UI_FILT_BW_LOW_LATENCY (15 << 4)
#define ICM426XX_GYRO_UI_FILT_BW_LOW_LATENCY (15 << 0)
// ----------------------------------------------------------

#define ICM426XX_RA_GYRO_DATA_X1 0x25  // User Bank 0
#define ICM426XX_RA_ACCEL_DATA_X1 0x1F // User Bank 0

#define ICM426XX_RA_INT_CONFIG 0x14 // User Bank 0
#define ICM426XX_INT1_MODE_PULSED (0 << 2)
#define ICM426XX_INT1_MODE_LATCHED (1 << 2)
#define ICM426XX_INT1_DRIVE_CIRCUIT_OD (0 << 1)
#define ICM426XX_INT1_DRIVE_CIRCUIT_PP (1 << 1)
#define ICM426XX_INT1_POLARITY_ACTIVE_LOW (0 << 0)
#define ICM426XX_INT1_POLARITY_ACTIVE_HIGH (1 << 0)

#define ICM426XX_RA_INT_CONFIG0 0x63 // User Bank 0
#define ICM426XX_UI_DRDY_INT_CLEAR_ON_SBR ((0 << 5) || (0 << 4))
#define ICM426XX_UI_DRDY_INT_CLEAR_ON_SBR_DUPLICATE ((0 << 5) || (0 << 4)) // duplicate settings in datasheet, Rev 1.2.
#define ICM426XX_UI_DRDY_INT_CLEAR_ON_F1BR ((1 << 5) || (0 << 4))
#define ICM426XX_UI_DRDY_INT_CLEAR_ON_SBR_AND_F1BR ((1 << 5) || (1 << 4))

#define ICM426XX_RA_INT_CONFIG1 0x64 // User Bank 0
#define ICM426XX_INT_ASYNC_RESET_BIT 4
#define ICM426XX_INT_TDEASSERT_DISABLE_BIT 5
#define ICM426XX_INT_TDEASSERT_ENABLED (0 << ICM426XX_INT_TDEASSERT_DISABLE_BIT)
#define ICM426XX_INT_TDEASSERT_DISABLED (1 << ICM426XX_INT_TDEASSERT_DISABLE_BIT)
#define ICM426XX_INT_TPULSE_DURATION_BIT 6
#define ICM426XX_INT_TPULSE_DURATION_100 (0 << ICM426XX_INT_TPULSE_DURATION_BIT)
#define ICM426XX_INT_TPULSE_DURATION_8 (1 << ICM426XX_INT_TPULSE_DURATION_BIT)

#define ICM426XX_RA_INT_SOURCE0 0x65 // User Bank 0
#define ICM426XX_UI_DRDY_INT1_EN_DISABLED (0 << 3)
#define ICM426XX_UI_DRDY_INT1_EN_ENABLED (1 << 3)

#define MPU_RA_WHO_AM_I 0x75 // should return 0x47

typedef enum
{
    ODR_CONFIG_8K = 0,
    ODR_CONFIG_4K,
    ODR_CONFIG_2K,
    ODR_CONFIG_1K,
    ODR_CONFIG_COUNT
} odrConfig_e;

typedef enum
{
    AAF_CONFIG_258HZ = 0,
    AAF_CONFIG_536HZ,
    AAF_CONFIG_997HZ,
    AAF_CONFIG_1962HZ,
    AAF_CONFIG_COUNT
} aafConfig_e;

typedef struct aafConfig_s
{
    uint8_t delt;
    uint16_t deltSqr;
    uint8_t bitshift;
} aafConfig_t;

// // Possible output data rates (ODRs)
// static const uint8_t odrLUT[ODR_CONFIG_COUNT] = {
//     // see GYRO_ODR in section 5.6
//     [ODR_CONFIG_8K] = 3,
//     [ODR_CONFIG_4K] = 4,
//     [ODR_CONFIG_2K] = 5,
//     [ODR_CONFIG_1K] = 6,
// };

// Possible gyro Anti-Alias Filter (AAF) cutoffs for ICM-42688P
static const aafConfig_t aafLUT42688[AAF_CONFIG_COUNT] = {
    // see table in section 5.3
    [AAF_CONFIG_258HZ] = {6, 36, 10},
    [AAF_CONFIG_536HZ] = {12, 144, 8},
    [AAF_CONFIG_997HZ] = {21, 440, 6},
    [AAF_CONFIG_1962HZ] = {37, 1376, 4},
};

// // Possible gyro Anti-Alias Filter (AAF) cutoffs for ICM-42688P
// // actual cutoff differs slightly from those of the 42688P
// static const aafConfig_t aafLUT42605[AAF_CONFIG_COUNT] = {
//     // see table in section 5.3
//     [AAF_CONFIG_258HZ] = {21, 440, 6},   // actually 249 Hz
//     [AAF_CONFIG_536HZ] = {39, 1536, 4},  // actually 524 Hz
//     [AAF_CONFIG_997HZ] = {63, 3968, 3},  // actually 995 Hz
//     [AAF_CONFIG_1962HZ] = {63, 3968, 3}, // 995 Hz is the max cutoff on the 42605
// };

//--------------------------------------------------------------------
/**
 * @brief Accelerometer scale options
 */
enum icm42688_accel_fs
{
    ICM42688_ACCEL_FS_16G,
    ICM42688_ACCEL_FS_8G,
    ICM42688_ACCEL_FS_4G,
    ICM42688_ACCEL_FS_2G,
};

/**
 * @brief Gyroscope scale options
 */
enum icm42688_gyro_fs
{
    ICM42688_GYRO_FS_2000,
    ICM42688_GYRO_FS_1000,
    ICM42688_GYRO_FS_500,
    ICM42688_GYRO_FS_250,
    ICM42688_GYRO_FS_125,
    ICM42688_GYRO_FS_62_5,
    ICM42688_GYRO_FS_31_25,
    ICM42688_GYRO_FS_15_625,
};
/**
 * @brief Accelerometer data rate options
 */
enum icm42688_accel_odr
{
    ICM42688_ACCEL_ODR_32000 = 1,
    ICM42688_ACCEL_ODR_16000,
    ICM42688_ACCEL_ODR_8000,
    ICM42688_ACCEL_ODR_4000,
    ICM42688_ACCEL_ODR_2000,
    ICM42688_ACCEL_ODR_1000,
    ICM42688_ACCEL_ODR_200,
    ICM42688_ACCEL_ODR_100,
    ICM42688_ACCEL_ODR_50,
    ICM42688_ACCEL_ODR_25,
    ICM42688_ACCEL_ODR_12_5,
    ICM42688_ACCEL_ODR_6_25,
    ICM42688_ACCEL_ODR_3_125,
    ICM42688_ACCEL_ODR_1_5625,
    ICM42688_ACCEL_ODR_500,
};
/**
 * @brief Gyroscope data rate options
 */
enum icm42688_gyro_odr
{
    ICM42688_GYRO_ODR_32000 = 1,
    ICM42688_GYRO_ODR_16000,
    ICM42688_GYRO_ODR_8000,
    ICM42688_GYRO_ODR_4000,
    ICM42688_GYRO_ODR_2000,
    ICM42688_GYRO_ODR_1000,
    ICM42688_GYRO_ODR_200,
    ICM42688_GYRO_ODR_100,
    ICM42688_GYRO_ODR_50,
    ICM42688_GYRO_ODR_25,
    ICM42688_GYRO_ODR_12_5,
    ICM42688_GYRO_ODR_500 = 0xF
};

static const float gscale[8] = {
    // see 3.1 GYROSCOPE SPECIFICATIONS
    [ICM42688_GYRO_FS_2000] = 16.4f,
    [ICM42688_GYRO_FS_1000] = 32.8f,
    [ICM42688_GYRO_FS_500] = 65.5f,
    [ICM42688_GYRO_FS_250] = 131.f,
    [ICM42688_GYRO_FS_125] = 262.f,
    [ICM42688_GYRO_FS_62_5] = 524.3f,
    [ICM42688_GYRO_FS_31_25] = 1048.6f,
    [ICM42688_GYRO_FS_15_625] = 2097.2f
};

static const float ascale[4] = {
    // see 3.2 ACCELEROMETER SPECIFICATIONS
    [ICM42688_ACCEL_FS_16G] = 2048.f,
    [ICM42688_ACCEL_FS_8G] = 4096.f,
    [ICM42688_ACCEL_FS_4G] = 8192.f,
    [ICM42688_ACCEL_FS_2G] = 16384.f
};



//------------------------------------------------------------------------
// -----------------Set acc/gryo full scale and odr Here------------------
static const enum icm42688_accel_fs acc_fs = ICM42688_ACCEL_FS_2G;
static const enum icm42688_gyro_fs gyr_fs = ICM42688_GYRO_FS_1000;
static const enum icm42688_accel_odr acc_odr = ICM42688_ACCEL_ODR_200;
static const enum icm42688_gyro_odr gyr_odr = ICM42688_GYRO_ODR_200;

// Write one register value
static int spi_write_register(spi_device_handle_t dev, uint8_t reg, uint8_t data)
{
    spi_transaction_t trans = {0};
    trans.tx_data[0] = reg;
    trans.tx_data[1] = data;
    trans.length = 8 * 2;   // 8bits * 2
    trans.flags = SPI_TRANS_USE_TXDATA;
    return spi_device_polling_transmit(dev, &trans);
}

// Read one register value
static int spi_read_register(spi_device_handle_t dev, uint8_t reg, uint8_t *data,
                                    size_t len)
{
    spi_transaction_t trans = {0};
    esp_err_t ret = 0;

    // When using SPI_TRANS_CS_KEEP_ACTIVE, bus must be locked/acquired
    spi_device_acquire_bus(dev, portMAX_DELAY);

    trans.tx_data[0] = REG_SPI_READ_BIT | reg;      // set register address and set read flag
    trans.length = 8;
    trans.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_CS_KEEP_ACTIVE;
    ret |= spi_device_polling_transmit(dev, &trans); // send address first
    assert(ret == ESP_OK);

    memset(trans.tx_data, 0 , 4);   // send empty bytes for burst reads
    trans.rxlength = 4;
    trans.length = 8 * 4;   // rx length = tx length in full duplex mode
    trans.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA | SPI_TRANS_CS_KEEP_ACTIVE;

    for (; len > 4; len -= 4)
    {
        if (len == 4) trans.flags &= ~SPI_TRANS_CS_KEEP_ACTIVE; // if this is the last reading, inactive cs when done
        spi_device_polling_transmit(dev, &trans);
        memcpy(data, trans.rx_data, 4);
        data += 4;
    }

    // read the part less than 4 bytes
    if (len > 0)
    {
        trans.flags &= ~SPI_TRANS_CS_KEEP_ACTIVE;
        ret |= spi_device_polling_transmit(dev, &trans);
        assert(ret == ESP_OK);
        memcpy(data, trans.rx_data, len);
    }

    spi_device_release_bus(dev);        // must realease before exit

    if (ret)
    {
        ESP_LOGW(IMU_TAG, "SPI READ ERR(%xd)", ret);
    }

    return ret;
}

static aafConfig_t getGyroAafConfig(const aafConfig_e);

static void turnGyroAccOff(spi_device_handle_t dev)
{
    spi_write_register(dev, ICM426XX_RA_PWR_MGMT0, ICM426XX_PWR_MGMT0_GYRO_ACCEL_MODE_OFF);
}

// Turn on gyro and acc on in Low Noise mode
static void turnGyroAccOn(spi_device_handle_t dev)
{
    spi_write_register(dev, ICM426XX_RA_PWR_MGMT0, ICM426XX_PWR_MGMT0_TEMP_DISABLE_OFF | ICM426XX_PWR_MGMT0_ACCEL_MODE_LN | ICM426XX_PWR_MGMT0_GYRO_MODE_LN);
    vTaskDelay(pdMS_TO_TICKS(1));
}

static void setUserBank(spi_device_handle_t dev, const uint8_t user_bank)
{
    spi_write_register(dev, ICM426XX_RA_REG_BANK_SEL, user_bank & 7);
}

int icm42688_Init(spi_device_handle_t dev)
{
    int ret = 0;
    uint8_t ID = 0;
    //check who am I
    ret |= spi_read_register(dev, MPU_RA_WHO_AM_I, &ID, 1);
    ESP_LOGI(IMU_TAG,"WHO AM I ID:0x%x", ID);
    if (ID != 0x47) // ICM42688 ID is 0x47
    {
        ESP_LOGE(IMU_TAG,"WHO_AM_I ID not match:0x%x", ID);
        return ESP_ERR_INVALID_MAC;
    }

    // Turn off ACC and GYRO so they can be configured
    // See section 12.9 in ICM-42688-P datasheet v1.7
    setUserBank(dev, ICM426XX_BANK_SELECT0);
    turnGyroAccOff(dev);

    // Configure gyro Anti-Alias Filter (see section 5.3 "ANTI-ALIAS FILTER")
    aafConfig_t aafConfig = getGyroAafConfig(AAF_CONFIG_258HZ);
    setUserBank(dev, ICM426XX_BANK_SELECT1);
    ret |= spi_write_register(dev, ICM426XX_RA_GYRO_CONFIG_STATIC3, aafConfig.delt);
    ret |= spi_write_register(dev, ICM426XX_RA_GYRO_CONFIG_STATIC4, aafConfig.deltSqr & 0xFF);
    ret |= spi_write_register(dev, ICM426XX_RA_GYRO_CONFIG_STATIC5, (aafConfig.deltSqr >> 8) | (aafConfig.bitshift << 4));

    // Configure acc Anti-Alias Filter for 1kHz sample rate (see tasks.c)
    aafConfig = getGyroAafConfig(AAF_CONFIG_258HZ);
    setUserBank(dev, ICM426XX_BANK_SELECT2);
    ret |= spi_write_register(dev, ICM426XX_RA_ACCEL_CONFIG_STATIC2, aafConfig.delt << 1);
    ret |= spi_write_register(dev, ICM426XX_RA_ACCEL_CONFIG_STATIC3, aafConfig.deltSqr & 0xFF);
    ret |= spi_write_register(dev, ICM426XX_RA_ACCEL_CONFIG_STATIC4, (aafConfig.deltSqr >> 8) | (aafConfig.bitshift << 4));

    // Configure gyro and acc UI Filters
    setUserBank(dev, ICM426XX_BANK_SELECT0);
    ret |= spi_write_register(dev, ICM426XX_RA_GYRO_ACCEL_CONFIG0, ICM426XX_ACCEL_UI_FILT_BW_LOW_LATENCY | ICM426XX_GYRO_UI_FILT_BW_LOW_LATENCY);

    ret |= spi_write_register(dev, ICM426XX_RA_INT_SOURCE0, ICM426XX_UI_DRDY_INT1_EN_DISABLED);

    // Turn on gyro and acc on again so ODR and FSR can be configured
    turnGyroAccOn(dev);

    // Set acc/gryo full scale and odr
    ret |= spi_write_register(dev, ICM426XX_RA_ACCEL_CONFIG0, acc_fs << 5 | (acc_odr & 0x0F));
    ret |= spi_write_register(dev, ICM426XX_RA_GYRO_CONFIG0, gyr_fs << 5 | (gyr_odr & 0x0F));

    if (ret != 0)
    {
        ESP_LOGE(IMU_TAG, "ICM42688 int failed Code:%d.", ret);
    }
    else
        ESP_LOGI(IMU_TAG, "ICM42688 int Success.");
    /*
     * Accelerometer sensor need at least 10ms startup time
     * Gyroscope sensor need at least 30ms startup time
     */
    vTaskDelay(pdMS_TO_TICKS(50));
    return ret;
}

int icm42688_AccGyr_fetch(spi_device_handle_t dev, uint8_t *reading, uint8_t len)
{
    int ret;

    ret = spi_read_register(dev, ICM426XX_RA_ACCEL_DATA_X1, reading, len);
    if (ret != 0)
    {
        ESP_LOGE(IMU_TAG, "ICM42688 fetch data failed Code:%d.", ret);
    }

    return ret;
}

void icm42688_acc_get(uint8_t *rawAccel, float *racc_array)
{
    float raw0 = (int16_t)((((int16_t)rawAccel[0]) << 8) | rawAccel[1]);
    float raw1 = (int16_t)((((int16_t)rawAccel[2]) << 8) | rawAccel[3]);
    float raw2 = (int16_t)((((int16_t)rawAccel[4]) << 8) | rawAccel[5]);
    racc_array[0] = raw0 / ascale[acc_fs];
    racc_array[1] = raw1 / ascale[acc_fs];
    racc_array[2] = raw2 / ascale[acc_fs];
}

void icm42688_gyr_get(uint8_t *rawGyro, float *rgyr_array)
{
    float raw0 = (int16_t)((((int16_t)rawGyro[0]) << 8) | rawGyro[1]);
    float raw1 = (int16_t)((((int16_t)rawGyro[2]) << 8) | rawGyro[3]);
    float raw2 = (int16_t)((((int16_t)rawGyro[4]) << 8) | rawGyro[5]);
    rgyr_array[0] = raw0 / gscale[gyr_fs];
    rgyr_array[1] = raw1 / gscale[gyr_fs];
    rgyr_array[2] = raw2 / gscale[gyr_fs];
}

static aafConfig_t getGyroAafConfig(const aafConfig_e config)
{
    switch (config)
    {
    case AAF_CONFIG_258HZ:
        return aafLUT42688[AAF_CONFIG_258HZ];
    case AAF_CONFIG_536HZ:
        return aafLUT42688[AAF_CONFIG_536HZ];
    case AAF_CONFIG_997HZ:
        return aafLUT42688[AAF_CONFIG_997HZ];
#ifdef USE_GYRO_DLPF_EXPERIMENTAL
    case GYRO_HARDWARE_LPF_EXPERIMENTAL:
        return aafLUT42688[AAF_CONFIG_1962HZ];
#endif
    default:
        return aafLUT42688[AAF_CONFIG_258HZ];
    }
}
