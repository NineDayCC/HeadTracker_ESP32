#ifdef HT_NANO
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/touch_pad.h"
#include "touch.h"

#define TOUCH_THRESH_NO_USE (0)
#define TOUCH_FILTER_MODE_EN (1)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)
/*
  Read values sensed at all available touch pads.
 Print out values in a loop on a serial monitor.
 */
// static void tp_example_read_task(void *pvParameter)
// {
//     uint16_t touch_value;
//     uint16_t touch_filter_value;
//     while (1) {
// #if TOUCH_FILTER_MODE_EN
//         // If open the filter mode, please use this API to get the touch pad count.
//         touch_pad_read_raw_data(PIN_TOUCH, &touch_value);
//         touch_pad_read_filtered(PIN_TOUCH, &touch_filter_value);
//         printf("%d,%d", touch_value, touch_filter_value);
// #else
//         touch_pad_read(i, &touch_value);
//         printf("[%4"PRIu16"] ", touch_value);
// #endif
//         printf("\n");
//         vTaskDelay(10 / portTICK_PERIOD_MS);
//     }
// }

void tp_example_touch_pad_init(void)
{
    touch_pad_config(PIN_TOUCH, TOUCH_THRESH_NO_USE);
}

void touch_Init(void)
{
    // Initialize touch pad peripheral.
    // The default fsm mode is software trigger mode.
    ESP_ERROR_CHECK(touch_pad_init());
    // Set reference voltage for charging/discharging
    // In this case, the high reference valtage will be 2.7V - 1V = 1.7V
    // The low reference voltage will be 0.5
    // The larger the range, the larger the pulse count value.
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    tp_example_touch_pad_init();
#if TOUCH_FILTER_MODE_EN
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
#endif
    // Start task to read values sensed by pads
    // xTaskCreatePinnedToCore(&tp_example_read_task, "touch_pad_read_task", 4096, NULL, 5, NULL, 1);
}

#elif defined HT_NANO_V2
void touch_Init()
{
}

#endif