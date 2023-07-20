#include "io.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "defines.h"
#include "multi_button/multi_button.h"

//------------------------------------------------------------------------------
// Defines
#define DT_CENTER_BUTTON DT_NODELABEL(center_button)

//------------------------------------------------------------------------------
// Macro Modules

LOG_MODULE_REGISTER(ioLog, LOG_LEVEL_DBG);
K_SEM_DEFINE(btn1_single_click_sem, 0, 1);
K_SEM_DEFINE(btn1_long_start_sem, 0, 1);

//------------------------------------------------------------------------------
// Private Values
static const struct gpio_dt_spec cnt_button = GPIO_DT_SPEC_GET(DT_CENTER_BUTTON, gpios);
static const uint8_t btn1_id = 0;
struct Button btn1;

//--------------------Function Defines--------------------

uint8_t read_button_GPIO(uint8_t button_id)
{
    // you can share the GPIO read function with multiple Buttons
    switch (button_id)
    {
    case btn1_id:
        return gpio_pin_get_dt(&cnt_button);
    default:
        return 0;
    }
}

/**
 * @brief Initialises the IO device.
 */
int io_Init(void)
{
    int ret;

    if (!gpio_is_ready_dt(&cnt_button))
    {
        LOG_ERR("Error: button device %s is not ready\r\n", cnt_button.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&cnt_button, GPIO_INPUT);
    if (ret != 0)
    {
        printk("Error %d: failed to configure %s pin %d\n",
               ret, cnt_button.port->name, cnt_button.pin);
        return 0;
    }

    button_init(&btn1, read_button_GPIO, 1, btn1_id);
    button_attach(&btn1, SINGLE_CLICK, BTN1_SINGLE_Click_Handler);
    button_attach(&btn1, LONG_PRESS_START, BTN1_LONG_PRESS_START_Handler);
    button_start(&btn1);
    return 0;
}

void io_Thread(void)
{
    static uint8_t btn1_event_val;
    while (true)
    {
        button_ticks(); // read button status

        k_msleep(IO_PERIOD);
    }
}

void BTN1_SINGLE_Click_Handler(void* btn)
{
    k_sem_give(&btn1_single_click_sem);
}

void BTN1_LONG_PRESS_START_Handler(void* btn)
{
    k_sem_give(&btn1_long_start_sem);
}

/**
 * @brief Did the center button single clicked?
 * @retval true: button was single clicked.
 * @retval false: button was NOT single clicked.
 */
bool isSingleClick(void)
{
    if (k_sem_take(&btn1_single_click_sem, K_NO_WAIT))
        return false;
    return true;
}

/**
 * @brief Is the center button in long start status?
 * @retval true: button is in long start status.
 * @retval false: button is NOT in long start status.
 */
bool isLongStart(void)
{
    if (k_sem_take(&btn1_long_start_sem, K_NO_WAIT))
        return false;
    return true;
}