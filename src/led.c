#if defined HT_NANO || defined HT_NANO_V2 || defined HT_SE || defined RX_SE
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "led.h"
#include "ht.h"
#include "io.h"

#define LED_UPDATE_PERIOD 5 // 5ms

#define T(x) (x / LED_UPDATE_PERIOD) // convert ms to LED update times

const uint8_t LEDSEQ_DISCONNECTED[] = {1, T(500), T(500)};          // 500ms off, 500ms on.
const uint8_t LEDSEQ_WIFI_UPDATE[] = {T(20), T(30)};                // 20ms on, 30ms off
const uint8_t LEDSEQ_BINDING[] = {T(100), T(100), T(100), T(1000)}; // 2x 100ms blink, 1s pause
const uint8_t LEDSEQ_CONNECTED[] = {0xFF};                          // solid on

#define SET_LED_ON() gpio_set_level(GPIO_LED_STATUS_SET, GPIO_LED_STATUS_SET_ACTIVE_LEVEL)     // set led on
#define SET_LED_OFF() gpio_set_level(GPIO_LED_STATUS_SET, !GPIO_LED_STATUS_SET_ACTIVE_LEVEL)   // set led on
#define TOGGLE_LED() gpio_set_level(GPIO_LED_STATUS_SET, !gpio_get_level(GPIO_LED_STATUS_SET)) // toggle led

typedef struct
{
    const uint8_t *effect_array;
    uint8_t effect_array_size;
    uint8_t effect_array_index;
} led_effect_t;

static led_status_t led_status = disconnected;
static led_effect_t led_effect = {NULL, 0, 0};

void led_set_status(led_status_t status)
{
    led_status = status;
}

void led_update(void)
{
    static led_status_t last_led_status = disconnected;
    static uint8_t effect_cnt = 0;

    if (led_status != last_led_status)
    {
        SET_LED_ON(); // reset led status first
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