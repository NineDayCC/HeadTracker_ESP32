#if defined HT_NANO || defined HT_NANO_V2 || defined HT_SE || defined RX_SE
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"

#include "led.h"
#include "ht.h"
#include "io.h"
#include "defines.h"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (GPIO_LED_STATUS) // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_1
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (1638)                // Set duty to 50%. (2 ** 13) * 20% = 1638
#define LEDC_FREQUENCY (2000)           // Frequency in Hertz. Set frequency at 2 kHz

#define LED_UPDATE_PERIOD 5 // 5ms

const uint8_t LEDSEQ_DISCONNECTED[] = {50, 50};     // 500ms off, 500ms on.
const uint8_t LEDSEQ_WIFI_UPDATE[] = {2, 3};        // 20ms on, 30ms off
const uint8_t LEDSEQ_BINDING[] = {10, 10, 10, 100}; // 2x 100ms blink, 1s pause
const uint8_t LEDSEQ_CONNECTED[] = {0xFF};          // solid on

typedef struct
{
    const uint8_t *effect_array;
    uint8_t effect_array_size;
    uint8_t effect_array_index;
} led_effect_t;

static bool is_led_on = false; // led status, true means led is on, false means led is off
static led_status_t led_status = disconnected;
static led_effect_t led_effect = {LEDSEQ_DISCONNECTED, sizeof(LEDSEQ_DISCONNECTED), 0};

// Set the buzzer peripheral configuration
void led_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LEDC_OUTPUT_IO,
        .duty = 0, // Set duty to 0%
        .hpoint = 0,
        .flags.output_invert = 1, // HT_SE is connected to a blue led.
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static inline void set_led_on(void)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY); // Set duty to 50%, LED is on
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    is_led_on = true; // update led status
}

static inline void set_led_off(void)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0); // Set duty to 0%
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    is_led_on = false; // update led status
}

static inline void TOGGLE_LED(void)
{
    if (is_led_on)
    {
        set_led_off(); // turn off led
    }
    else
    {
        set_led_on(); // turn on led
    }
}

void led_set_status(led_status_t status)
{
    led_status = status;
}

void led_update(void)
{
    static led_status_t last_led_status = disconnected;
    static uint8_t effect_cnt = 0;
    static bool skip_flg = false; // double the period. 5ms * 2 = 10ms

    skip_flg = !skip_flg; // skip every other update to reduce flicker
    if (skip_flg)
    {
        return; // skip this update
    }

    if (led_status != last_led_status)
    {
        if (led_status == disconnected)
        {
            set_led_off(); // turn off led first if disconnected. So it would be easier to find disconnection issue.
        }
        else
        {
            set_led_on(); // reset led status first
        }

        last_led_status = led_status;
        led_effect.effect_array_index = 0; // reset effect array index
        effect_cnt = 0;                    // reset effect count
        switch (led_status)
        {
        case connected:
            led_effect.effect_array = LEDSEQ_CONNECTED;
            led_effect.effect_array_size = sizeof(LEDSEQ_CONNECTED); // solid on
            break;
        case disconnected:
            led_effect.effect_array = LEDSEQ_DISCONNECTED;
            led_effect.effect_array_size = sizeof(LEDSEQ_DISCONNECTED);
            break;
        case binding:
            led_effect.effect_array = LEDSEQ_BINDING;
            led_effect.effect_array_size = sizeof(LEDSEQ_BINDING);
            break;
        case ota:
            led_effect.effect_array = LEDSEQ_WIFI_UPDATE;
            led_effect.effect_array_size = sizeof(LEDSEQ_WIFI_UPDATE);
            break;
        default:
            led_effect.effect_array = LEDSEQ_DISCONNECTED;
            led_effect.effect_array_size = sizeof(LEDSEQ_DISCONNECTED);
            break;
        }
    }
    else if (led_status == connected)
    {
        return; // led is connected, solid on, no need to update
    }

    if (led_effect.effect_array_index >= led_effect.effect_array_size) // check if effect array index is out of range
    {
        led_effect.effect_array_index = 0; // reset effect array index
    }

    if (led_effect.effect_array == NULL)
    {
        ESP_LOGE("LED", "Effect array is NULL, please set led status first.");
        return; // effect array is not set, return
    }

    if (effect_cnt >= led_effect.effect_array[led_effect.effect_array_index])
    {
        TOGGLE_LED();   // toggle led
        effect_cnt = 0; // reset effect count
        led_effect.effect_array_index++;
        if (led_effect.effect_array_index >= led_effect.effect_array_size)
        {
            led_effect.effect_array_index = 0; // reset effect array index
        }
    }

    effect_cnt++;
}

#endif