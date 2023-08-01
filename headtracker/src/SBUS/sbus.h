#ifndef SBUS_H
#define SBUS_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>

int sbusInit(const struct device *dev);
void sbusBuildChannels(uint16_t *ch_);
void sbusTX(const struct device *dev);

#endif