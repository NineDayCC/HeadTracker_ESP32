#include "uart.h"

#include <zephyr/kernel.h>
#include <stdio.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#include "trackersettings.h"
#include "sbus/sbus.h"

//------------------------------------------------------------------------------
// Defines
#define UART_NODE1 DT_NODELABEL(uart1)

//------------------------------------------------------------------------------
// Macro Modules
LOG_MODULE_REGISTER(UartLog, LOG_LEVEL_DBG);

//------------------------------------------------------------------------------
// Private Values
const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE1);

static struct k_poll_signal uartThreadRunSignal = K_POLL_SIGNAL_INITIALIZER(uartThreadRunSignal);
struct k_poll_event uartRunEvents[1] = {
    K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &uartThreadRunSignal),
};

void uart_init()
{
    if (!device_is_ready(uart_dev))
    {
        printk("UART device not found!");
        return;
    }
    sbusInit(uart_dev);
    k_poll_signal_raise(&uartThreadRunSignal, 1);
}

void uart_Thread()
{
    while (true)
    {
        k_poll(uartRunEvents, 1, K_FOREVER);

        sbusTX(uart_dev);

        k_usleep(UART_PERIOD); // Hight speed mode 4ms
    }
}
