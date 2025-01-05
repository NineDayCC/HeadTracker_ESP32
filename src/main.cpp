#ifdef FRAMEWORK_ARDUINO
#include <Arduino.h>
#include "receiver.h"
#include "ppm.h"
#include "app_espnow.h"
#include <Ticker.h>
#include "multi_button.h"

static Ticker button_loop_timer;
static struct Button btn_bind;
static const uint8_t btn_bind_id = 0;

static uint8_t read_button_GPIO(uint8_t button_id)
{
    switch (button_id)
    {
    case btn_bind_id:
        return digitalRead(GPIO_BIND_BUTTON);
    default:
        return 0;
    }
}

static void BTN_SINGLE_Click_Handler(void *btn)
{
    Serial.println("Switch LED.");
    digitalWrite(GPIO_LED_STATUS, !digitalRead(GPIO_LED_STATUS));
}

static void BTN_LONG_PRESS_START_Handler(void *btn)
{
    if (!isBinding())
    {
        set_binding_mode(true);
    }
}

void setup()
{
    // Initialize pin
    pinMode(GPIO_LED_STATUS, OUTPUT);
    pinMode(GPIO_BIND_BUTTON, INPUT_PULLUP);
    digitalWrite(GPIO_LED_STATUS, LOW); // Led on

    Serial.begin(921600);

    button_init(&btn_bind, read_button_GPIO, 0, btn_bind_id);
    button_attach(&btn_bind, SINGLE_CLICK, BTN_SINGLE_Click_Handler);
    button_attach(&btn_bind, LONG_PRESS_START, BTN_LONG_PRESS_START_Handler);
    button_start(&btn_bind);
    button_loop_timer.attach_ms(5, button_ticks);

    PPMinit();
    rx_espnow_init();
    Serial.println("SATRT");
}

void loop()
{
    rx_espnow_loop();
}

#endif