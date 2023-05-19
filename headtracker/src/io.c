#include "io.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "defines.h"

//------------------------------------------------------------------------------
// Macro Modules

LOG_MODULE_REGISTER(ioLog, LOG_LEVEL_DBG);
K_SEM_DEFINE(button_pressed_sem, 0, 1);


/**
 * @brief Initialises the IO device.
 */
int io_Init(void)
{
    int ret;

    const struct gpio_dt_spec cnt_button = GPIO_DT_SPEC_GET(DT_NODELABEL(center_button), gpios);
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

    return 0;
}

void io_Thread(void)
{
    const struct gpio_dt_spec cnt_button = GPIO_DT_SPEC_GET(DT_NODELABEL(center_button), gpios);
    static int previouStatus;
    while (true)
    {
        int status;
        status = gpio_pin_get_dt(&cnt_button);
        if (status == 1 && previouStatus == 0)
        {
            pressButton();
            // LOG_INF("Button Pressed");
        }
        else if (status == 0 && previouStatus == 1)
        {
            // LOG_INF("Button Released");
        }

        previouStatus = status;
        k_msleep(IO_PERIOD);
    }
}

void pressButton()
{
    k_sem_give(&button_pressed_sem);
}

// Reset Button Pressed Flag on Read
bool wasButtonPressed()
{
    if (k_sem_take(&button_pressed_sem, K_NO_WAIT))
        return false;
    return true;
}