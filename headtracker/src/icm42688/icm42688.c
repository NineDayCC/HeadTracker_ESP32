/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Author: Dominic Clifton
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#include "icm42688.h"

//------------------------------------------------------------------------------
// Defines
#define REG_SPI_READ_BIT BIT(7)

//------------------------------------------------------------------------------
// Macro Modules
LOG_MODULE_REGISTER(icm42688Log, LOG_LEVEL_DBG);

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

// Possible output data rates (ODRs)
static const uint8_t odrLUT[ODR_CONFIG_COUNT] = {
    // see GYRO_ODR in section 5.6
    [ODR_CONFIG_8K] = 3,
    [ODR_CONFIG_4K] = 4,
    [ODR_CONFIG_2K] = 5,
    [ODR_CONFIG_1K] = 6,
};

// Possible gyro Anti-Alias Filter (AAF) cutoffs for ICM-42688P
static const aafConfig_t aafLUT42688[AAF_CONFIG_COUNT] = {
    // see table in section 5.3
    [AAF_CONFIG_258HZ] = {6, 36, 10},
    [AAF_CONFIG_536HZ] = {12, 144, 8},
    [AAF_CONFIG_997HZ] = {21, 440, 6},
    [AAF_CONFIG_1962HZ] = {37, 1376, 4},
};

// Possible gyro Anti-Alias Filter (AAF) cutoffs for ICM-42688P
// actual cutoff differs slightly from those of the 42688P
static const aafConfig_t aafLUT42605[AAF_CONFIG_COUNT] = {
    // see table in section 5.3
    [AAF_CONFIG_258HZ] = {21, 440, 6},   // actually 249 Hz
    [AAF_CONFIG_536HZ] = {39, 1536, 4},  // actually 524 Hz
    [AAF_CONFIG_997HZ] = {63, 3968, 3},  // actually 995 Hz
    [AAF_CONFIG_1962HZ] = {63, 3968, 3}, // 995 Hz is the max cutoff on the 42605
};

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

// -----------------Set acc/gryo full scale and odr------------------
static const enum icm42688_accel_fs acc_fs = ICM42688_ACCEL_FS_2G;
static const enum icm42688_gyro_fs gyr_fs = ICM42688_GYRO_FS_2000;
static const enum icm42688_accel_odr acc_odr = ICM42688_ACCEL_ODR_1000;
static const enum icm42688_gyro_odr gyr_odr = ICM42688_GYRO_ODR_1000;

static const struct spi_config spi_cfg = {
    .frequency = 8000000U,
    .operation = SPI_WORD_SET(8),
    .slave = 0
};

// zephyr spi access
static inline int spi_write_register(const struct device *dev, uint8_t reg, uint8_t data)
{
    const struct spi_buf buf[2] = {
        {
            .buf = &reg,
            .len = 1,
        },
        {
            .buf = &data,
            .len = 1,
        }};

    const struct spi_buf_set tx = {
        .buffers = buf,
        .count = 2,
    };

    return spi_write(dev, &spi_cfg, &tx);
}

// zephyr spi access
static inline int spi_read_register(const struct device *dev, uint8_t reg, uint8_t *data,
                                    size_t len)
{
    uint8_t tx_buffer = REG_SPI_READ_BIT | reg;

    const struct spi_buf tx_buf = {
        .buf = &tx_buffer,
        .len = 1,
    };

    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1,
    };

    struct spi_buf rx_buf[2] = {
        {
            .buf = NULL,
            .len = 1,
        },
        {
            .buf = data,
            .len = len,
        }};

    const struct spi_buf_set rx = {
        .buffers = rx_buf,
        .count = 2,
    };

    return spi_transceive(dev, &spi_cfg, &tx, &rx);
}

static aafConfig_t getGyroAafConfig(const aafConfig_e);

static void turnGyroAccOff(const struct device *dev)
{
    spi_write_register(dev, ICM426XX_RA_PWR_MGMT0, ICM426XX_PWR_MGMT0_GYRO_ACCEL_MODE_OFF);
}

// Turn on gyro and acc on in Low Noise mode
static void turnGyroAccOn(const struct device *dev)
{
    spi_write_register(dev, ICM426XX_RA_PWR_MGMT0, ICM426XX_PWR_MGMT0_TEMP_DISABLE_OFF | ICM426XX_PWR_MGMT0_ACCEL_MODE_LN | ICM426XX_PWR_MGMT0_GYRO_MODE_LN);
    k_busy_wait(1000);
}

static void setUserBank(const struct device *dev, const uint8_t user_bank)
{
    spi_write_register(dev, ICM426XX_RA_REG_BANK_SEL, user_bank & 7);
}

int icm42688_Init(const struct device *dev)
{
    int ret = 0;
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

    // Configure interrupt pin
    // ret |= spi_write_register(dev, ICM426XX_RA_INT_CONFIG, ICM426XX_INT1_MODE_PULSED | ICM426XX_INT1_DRIVE_CIRCUIT_PP | ICM426XX_INT1_POLARITY_ACTIVE_HIGH);
    // ret |= spi_write_register(dev, ICM426XX_RA_INT_CONFIG0, ICM426XX_UI_DRDY_INT_CLEAR_ON_SBR);

    // ret |= spi_write_register(dev, ICM426XX_RA_INT_SOURCE0, ICM426XX_UI_DRDY_INT1_EN_ENABLED);

    // uint8_t intConfig1Value = spiReadRegMsk(dev, ICM426XX_RA_INT_CONFIG1);
    // // Datasheet says: "User should change setting to 0 from default setting of 1, for proper INT1 and INT2 pin operation"
    // intConfig1Value &= ~(1 << ICM426XX_INT_ASYNC_RESET_BIT);
    // intConfig1Value |= (ICM426XX_INT_TPULSE_DURATION_8 | ICM426XX_INT_TDEASSERT_DISABLED);

    // ret |= spi_write_register(dev, ICM426XX_RA_INT_CONFIG1, intConfig1Value);

    ret |= spi_write_register(dev, ICM426XX_RA_INT_SOURCE0, ICM426XX_UI_DRDY_INT1_EN_DISABLED);

    // Turn on gyro and acc on again so ODR and FSR can be configured
    turnGyroAccOn(dev);

    // Set acc/gryo full scale and odr
    ret |= spi_write_register(dev, ICM426XX_RA_ACCEL_CONFIG0, acc_fs << 5 | (acc_odr & 0x0F));
    ret |= spi_write_register(dev, ICM426XX_RA_GYRO_CONFIG0, gyr_fs << 5 | (gyr_odr & 0x0F));

    if (ret != 0)
    {
        LOG_ERR("ICM42688 int failed Code:%d.", ret);
    }
    else
        LOG_DBG("ICM42688 int Success.");
    /*
     * Accelerometer sensor need at least 10ms startup time
     * Gyroscope sensor need at least 30ms startup time
     */
    k_msleep(50);
    return ret;
}

int icm42688_AccGyr_fetch(const struct device *dev, uint8_t *reading, uint8_t len)
{
    int ret;

    ret = spi_read_register(dev, ICM426XX_RA_ACCEL_DATA_X1, reading, len);
    if (ret != 0)
    {
        LOG_ERR("ICM42688 fetch data failed Code:%d.", ret);
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
