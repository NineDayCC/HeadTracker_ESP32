#ifdef FRAMEWORK_ARDUINO
#include "ppm.h"
#include "defines.h"
#include "imu.h"
#include "trackersettings.h"

// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG 0
#define _TIMERINTERRUPT_LOGLEVEL_ 0

// Select a Timer Clock
#define USING_TIM_DIV1 true    // for shortest and most accurate timer
#define USING_TIM_DIV16 false  // for medium time and medium accurate timer
#define USING_TIM_DIV256 false // for longest timer but least accurate. Default

#include "ESP8266TimerInterrupt.h"
//------------------------------------------------------------------------------
// Defines
#define PPM_PIN GPIO_PPM_OUT_SET // shorter macro name
#define PPM_INIT_DELAY 500000    // ppm start delay in us
#define PPM_COMPENSATE 14        // Compensate for interrupt latency

//------------------------------------------------------------------------------

// Init ESP8266 timer 1
static ESP8266Timer ITimer;

// Used to read data at once, read with isr disabled
static uint16_t ch_values[16];
static int ch_count;

static uint16_t framesync = PPM_MIN_FRAMESYNC; // Minimum Frame Sync Pulse
static uint32_t framelength;                   // Ideal frame length
static uint16_t sync;                          // Sync Pulse Length

// Local data - Only build with interrupts disabled
static uint32_t chsteps[25] = {PPM_MIN_FRAMESYNC, PPM_MIN_SYNC};

// ISR Values - Values from chsteps are copied here on step 0.
// prevent an update from happening mid stream.
static uint32_t isrchsteps[25] = {PPM_MIN_FRAMESYNC, PPM_MIN_SYNC};
static uint16_t chstepcnt = 1;
static uint16_t curstep = 0;
volatile bool buildingdata = false;

/**
 * @brief PPM timer callback function.
 */
void IRAM_ATTR ppm_counter_interrupt_fn(void)
{
    static bool ppm_pin_status = 0;

    // Reset, don't get stuck in the wrong signal level
    if (curstep == 0)
    {
        // set the first output signal level
        ppm_pin_status = isPPMininvert();
        digitalWrite(PPM_PIN, ppm_pin_status);
    }
    // Not last frame
    else if (curstep < chstepcnt - 1)
    {
        digitalWrite(PPM_PIN, !ppm_pin_status); // toggle level
        ppm_pin_status = !ppm_pin_status;
    }
    // Don't toggle pin at last frame.
    // Loop
    else if (curstep > chstepcnt)
    {
        if (!buildingdata)
            memcpy(isrchsteps, chsteps, sizeof(chsteps));
        curstep = 0;
        buildChannels();
    }

    // Set next alarm
    ITimer.setInterval(isrchsteps[curstep] - PPM_COMPENSATE, ppm_counter_interrupt_fn);
    curstep++;
}

int PPMinit(void)
{
    ch_count = getPPMchcnt() > 10 ? 10 : getPPMchcnt();
    sync = getPPMsync();
    framelength = getPPMframe();
    bool ret = 0;

    // config gpio
    pinMode(PPM_PIN, OUTPUT);
    digitalWrite(PPM_PIN, LOW);

    // Set all channels to center
    for (int i = 0; i < 16; i++)
        ch_values[i] = 1500;

    // Init counter data
    buildChannels();
    memcpy(isrchsteps, chsteps, sizeof(chsteps));

    // Start ppm counter
    ret = ITimer.attachInterruptInterval(isrchsteps[0], ppm_counter_interrupt_fn);
    return !ret;
}

/**
 * @brief Build timer data for isr to change IO according to Channel data(pwm values).
 */
void buildChannels(void)
{
    buildingdata = true;

    // Set user defined channel count, frame len, sync pulse
    ch_count = getPPMchcnt() > 10 ? 10 : getPPMchcnt();
    sync = getPPMsync();
    framelength = getPPMframe();

    int ch = 0;
    int i;
    chsteps[0] = framesync;
    for (i = 1; i <= ch_count * 2 + 1; i += 2)
    {
        chsteps[i] = sync; // 300us sync pulse
        chsteps[i + 1] = ch_values[ch++] - sync;
    }
    // Add Final Sync
    chsteps[i++] = sync;
    chstepcnt = i;
    // Now we know how long the train is. Try to make the entire frame == framelength
    // If possible it will add this to the frame sync pulse
    uint32 total_time = 0;
    for (i = 0; i < chstepcnt; i++)
        total_time += chsteps[i];

    if (framelength >= total_time)
        chsteps[chstepcnt] = framelength - total_time; // Store at end of sequence
    else
        chsteps[chstepcnt] = 0; // Not possible, no time left
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

uint16_t PpmOut_getChannel(uint16_t chan)
{
    return ch_values[chan];
}

#endif