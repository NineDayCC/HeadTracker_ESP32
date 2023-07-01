#ifndef BT_H
#define BT_H

#include <zephyr/kernel.h>
#include "defines.h"


void bt_Thread();
void bt_init();

/**
 * @brief Build channel data for Bluetooth to sent
 * @param channelData pointer to the source channel data to be converted
 * @param channels the number of channels to be converted
 */
void buildBtChannels(uint16_t *channelData, uint8_t channels);



#endif