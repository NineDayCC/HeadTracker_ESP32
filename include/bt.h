#ifndef BT_H
#define BT_H

#include "defines.h"

#define DEVICE_NAME "HeadTracker_ESP32"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define BT_HEADTRACKER_UUID_VALUE 0x4EF0
#define BT_PWM_UUID_VALUE 0x4EF1

#define BT_THREAD_PRIORITY_SET      BT_THREAD_PRIORITY  //thread priority
#define BT_THREAD_STACK_SIZE_SET    BT_THREAD_STACK_SIZE  //thread stack size

void bt_Thread();
void bt_tx_init();
void bt_rx_init();
/**
 * @brief Build channel data for Bluetooth to sent
 * @param channelData pointer to the source channel data to be converted
 * @param channels the number of channels to be converted
 */
void buildBtChannels(uint16_t *channelData, uint8_t channels);



#endif