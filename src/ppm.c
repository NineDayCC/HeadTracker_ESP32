#ifndef HT_NANO
#include "ppm.h"

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"


#include "defines.h"
#include "imu.h"
#include "trackersettings.h"

//------------------------------------------------------------------------------
// Defines
#define PPM_PIN         GPIO_PPM_OUT_SET    // shorter macro name
#define PPM_INIT_DELAY  500000   // ppm start delay in us


//------------------------------------------------------------------------------
// Private Values
static const char* PPM_TAG = "PPM";

static gptimer_handle_t ppm_timer = NULL;
// struct counter_alarm_cfg alarm_cfg;

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
static bool IRAM_ATTR ppm_counter_interrupt_fn(gptimer_handle_t timer,
                                        const gptimer_alarm_event_data_t *edata,
                                        void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    uint64_t now_ticks;
    static uint32_t ticks_offset = 0;   // the first alarm value of each ppm duration
    static bool ppm_pin_status = 0;

    now_ticks = edata->count_value; // get current count value

    // Reset, don't get stuck in the wrong signal level
    if (curstep == 0)
    {
        // set the first output signal level
        if (isPPMininvert())
        {
            gpio_set_level(PPM_PIN, 1);
            ppm_pin_status = 1;
        }
        else
        {
            gpio_set_level(PPM_PIN, 0);
            ppm_pin_status = 0;
        }
    }
    // Not last frame
    else if (curstep < chstepcnt - 1)
    {
        gpio_set_level(PPM_PIN, !ppm_pin_status);  // toggle level
        ppm_pin_status = !ppm_pin_status;
    }

    // Setup next capture event value
    gptimer_alarm_config_t alarm_config = {0};
    // reconfigure alarm value
    alarm_config.alarm_count = ticks_offset + isrchsteps[curstep];

    if (now_ticks > alarm_config.alarm_count)  // alarm count smaller than the current count
    {
        // Reset offset, usually happened at the beginning
        ticks_offset = (curstep > 0) ? (now_ticks - isrchsteps[curstep - 1]) : now_ticks;
        alarm_config.alarm_count = ticks_offset + isrchsteps[curstep];
    }

    curstep++;
    // Loop
    if (curstep > chstepcnt)
    {
        if (!buildingdata)
            memcpy(isrchsteps, chsteps, sizeof(isrchsteps[0]) * 35);
        curstep = 0;
        ticks_offset = alarm_config.alarm_count;
    }

    /* Set a new alarm with a double length duration */
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));

    // printf("[%s]", now_str());
    // printf("step: %d\n", curstep);
    // printf("ticks: %lld\n", now_ticks);
    // printf("over: %d\n", ticks_over_flag);
    // printf("next ticks: %lld\n", alarm_config.alarm_count);
    // counter_get_value(counter_dev, &now_ticks);
    // printf("now ticks: %d\n", now_ticks);
    // uint32_t buff = now_ticks;
    // counter_get_value(counter_dev, &now_ticks);
    // printf("elipsed: %d\n", now_ticks - buff);
    return (high_task_awoken == pdTRUE);
}

int PPMinit(void)
{
    ch_count = getPPMchcnt();
    sync = getPPMsync();
    framelength = getPPMframe();

    //config gpio
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;  // disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;         // set as output mode
    io_conf.pin_bit_mask = (1ULL<<GPIO_PPM_OUT_SET);  // PPM output pin
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // config hardware timer
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &ppm_timer));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = ppm_counter_interrupt_fn,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(ppm_timer, &cbs, NULL));

    ESP_LOGI(PPM_TAG, "Enable timer");
    ESP_ERROR_CHECK(gptimer_enable(ppm_timer));

    ESP_LOGI(PPM_TAG, "Start timer, auto-reload at alarm event");
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = PPM_INIT_DELAY, // ppm start after 500ms
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(ppm_timer, &alarm_config));


    // Set all channels to center
    for (int i = 0; i < 16; i++)
        ch_values[i] = 1500;

    // Init counter data
    buildChannels();
    printPPMdata(); // test
    memcpy(isrchsteps, chsteps, sizeof(isrchsteps[0]) * 35);

    // Start ppm counter
    ESP_ERROR_CHECK(gptimer_start(ppm_timer));

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
        printf("%ld ", chsteps[i]);
    }
    printf("\n");
}

#endif