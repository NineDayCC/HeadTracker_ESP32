#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "application.h"


int main(void)
{
	printk("HeaderTracker ESP32 by NineDayCC.\nBoard: %s\n", CONFIG_BOARD);

	start();

	return 0;
}


