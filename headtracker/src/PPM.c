#include "PPM.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "defines.h"
#include "imu.h"
#include "trackersettings.h"

//------------------------------------------------------------------------------
// Defines
#define DT_TIMER DT_NODELABEL(timer0)
#define DT_PPM_PIN DT_NODELABEL(ppm_pin)
#define ALARM_CHANNEL_ID 0
#define DELAY 100000

//------------------------------------------------------------------------------
// Macro Modules

LOG_MODULE_REGISTER(ppmLog, LOG_LEVEL_DBG);

//------------------------------------------------------------------------------
// Private Values
struct counter_alarm_cfg alarm_cfg;

// Used to read data at once, read with isr disabled
static uint16_t ch_values[16];
static int ch_count;

static uint16_t framesync = PPM_MIN_FRAMESYNC; // Minimum Frame Sync Pulse
static int32_t framelength;                    // Ideal frame length
static uint16_t sync;                          // Sync Pulse Length

// Local data - Only build with interrupts disabled
static uint32_t chsteps[35] = {PPM_MIN_FRAMESYNC, PPM_MIN_SYNC};

// ISR Values - Values from chsteps are copied here on step 0.
// prevent an update from happening mid stream.
static uint32_t isrchsteps[35] = {PPM_MIN_FRAMESYNC, PPM_MIN_SYNC};
static uint16_t chstepcnt = 1;
static uint16_t curstep = 0;
volatile bool buildingdata = false;

/**
 * @brief PPM timer callback function.
 */
static void ppm_counter_interrupt_fn(const struct device *counter_dev,
                                     uint8_t chan_id, uint32_t ticks,
                                     void *user_data)
{
    static const struct gpio_dt_spec ppm_dev = GPIO_DT_SPEC_GET(DT_PPM_PIN, gpios);
    struct counter_alarm_cfg *config = user_data;
    uint32_t now_ticks;
    static uint32_t ticks_offset = 0;
    int err;

    err = counter_get_value(counter_dev, &now_ticks);
    if (err)
    {
        LOG_ERR("Failed to read counter value (err %d)", err);
        return;
    }

    // Reset, don't get stuck in the wrong signal level
    if (curstep == 0)
    {
        // set the first output signal level
        if (isPPMininvert())
            gpio_pin_set_dt(&ppm_dev, 1);
        else
            gpio_pin_set_dt(&ppm_dev, 0);
    }
    // Not last frame
    else if (curstep < chstepcnt - 1)
    {
        gpio_pin_toggle_dt(&ppm_dev);
    }

    // Setup next capture event value
    // int ticks_over_flag = 0; // test
    config->ticks = ticks_offset + isrchsteps[curstep];
    if (now_ticks > config->ticks)
    {
        // ticks_over_flag = 1; // test
        // Reset offset, usually happened at the beginning
        ticks_offset = (curstep > 0) ? (now_ticks - isrchsteps[curstep - 1]) : now_ticks;
        config->ticks = ticks_offset + isrchsteps[curstep];
    }

    curstep++;
    // Loop
    if (curstep > chstepcnt)
    {
        if (!buildingdata)
            memcpy(isrchsteps, chsteps, sizeof(isrchsteps[0]) * 35);
        curstep = 0;
        ticks_offset = config->ticks;
    }

    /* Set a new alarm with a double length duration */
    err = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
                                    user_data);
    if (err != 0)
    {
        LOG_ERR("Alarm could not be set");
    }

    // printk("[%s]", now_str());
    // printk("step: %d\n", curstep);
    // printk("ticks: %d\n", now_ticks);
    // printk("over: %d\n", ticks_over_flag);
    // printk("next ticks: %d\n", config->ticks);
    // counter_get_value(counter_dev, &now_ticks);
    // printk("now ticks: %d\n", now_ticks);
    // uint32_t buff = now_ticks;
    // counter_get_value(counter_dev, &now_ticks);
    // printk("elipsed: %d\n", now_ticks - buff);
}

int PPMinit(void)
{
    ch_count = getPPMchcnt();
    sync = getPPMsync();
    framelength = getPPMframe();

    int ret;

    const struct gpio_dt_spec ppm_dev = GPIO_DT_SPEC_GET(DT_PPM_PIN, gpios);
    if (!gpio_is_ready_dt(&ppm_dev))
    {
        LOG_ERR("Error: button device %s is not ready\r\n", ppm_dev.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&ppm_dev, GPIO_OUTPUT);
    if (ret != 0)
    {
        LOG_ERR("Error %d: failed to configure %s pin %d\r\n",
                ret, ppm_dev.port->name, ppm_dev.pin);
        return 0;
    }

    const struct device *const counter_dev = DEVICE_DT_GET(DT_TIMER);
    if (!device_is_ready(counter_dev))
    {
        LOG_ERR("timer not ready.\r\n");
        return 0;
    }

    // Set all channels to center
    for (int i = 0; i < 16; i++)
        ch_values[i] = 1500;

    // Init counter data
    buildChannels();
    // printPPMdata(); // test
    memcpy(isrchsteps, chsteps, sizeof(isrchsteps[0]) * 35);

    // Start ppm counter
    counter_start(counter_dev);

    alarm_cfg.flags = 1;
    alarm_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY);
    alarm_cfg.callback = ppm_counter_interrupt_fn;
    alarm_cfg.user_data = &alarm_cfg;

    // LOG_DBG("alarm freq:%d\n", counter_get_frequency(counter_dev)); // test
    ret = counter_set_channel_alarm(counter_dev, ALARM_CHANNEL_ID,
                                    &alarm_cfg);
    if (-EINVAL == ret)
    {
        LOG_ERR("Alarm settings invalid\r\n");
    }
    else if (-ENOTSUP == ret)
    {
        LOG_ERR("Alarm setting request not supported\r\n");
    }
    else if (ret != 0)
    {
        LOG_ERR("Error\r\n");
    }

    return 0;
}

/**
 * @brief Build timer data for isr to change IO according to Channel data(pwm values).
 */
void buildChannels(void)
{
    buildingdata = true; // Prevent a read happing while this is building

    // Set user defined channel count, frame len, sync pulse
    ch_count = getPPMchcnt();
    sync = getPPMsync();
    framelength = getPPMframe();

    int ch = 0;
    int i;
    uint32_t curtime = framesync;
    chsteps[0] = curtime;
    for (i = 1; i <= ch_count * 2 + 1; i += 2)
    {
        curtime += sync;
        chsteps[i] = curtime;
        curtime += (ch_values[ch++] - sync);
        chsteps[i + 1] = curtime;
    }
    // Add Final Sync
    curtime += sync;
    chsteps[i++] = curtime;
    chstepcnt = i;
    // Now we know how long the train is. Try to make the entire frame == framelength
    // If possible it will add this to the frame sync pulse

    if (framelength >= curtime)
        chsteps[i] = framelength; // Store at end of sequence
    else
        chsteps[i] = 0; // Not possible, no time left
    buildingdata = false;
}

void PpmOut_setChannel(int chan, uint16_t val)
{
    if (chan >= 0 && chan <= ch_count && val >= MIN_PWM &&
        val <= MAX_PWM)
    {
        ch_values[chan] = val;
    }
}

int PpmOut_getChnCount() { return ch_count; }

// test function
void printPPMdata(void)
{
    for (uint8_t i = 0; i < sizeof(chsteps) / sizeof(chsteps[0]); i++)
    {
        printf("%d ", chsteps[i]);
    }
    printf("\r\n");
}