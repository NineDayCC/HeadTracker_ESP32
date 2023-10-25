#ifndef ICM42688_H
#define ICM42688_H

/**
 * @brief Initiate icm42688 config
 * @param dev pointer to device
 * @return 0 if success, others if failed
 */
int icm42688_Init(spi_device_handle_t dev);

/**
 * @brief read accelerator and gyro spi data
 * @param dev pointer to device
 * @param reading pointer to data to be stored
 * @param len length of data to read
 * @return 0 if success, others if failed
 */
int icm42688_AccGyr_fetch(spi_device_handle_t dev, uint8_t *reading, uint8_t len);

/**
 * @brief get accelerator data in g from spi data
 * @param rawAccel pointer to spi data
 * @param racc_array pointer to accelerator data in g to be stored
 */
void icm42688_acc_get(uint8_t *rawAccel, float *racc_array);

/**
 * @brief get gyro data in g from spi data
 * @param rawGyro pointer to spi data
 * @param rgyr_array pointer to gyro data in g to be stored
 */
void icm42688_gyr_get(uint8_t *rawGyro, float *rgyr_array);

#endif