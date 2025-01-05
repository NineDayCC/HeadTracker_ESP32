#ifndef FRAMEWORK_ARDUINO

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "io.h"
#include "multi_button.h"
#include "touch.h"
#include "buzzer.h"
#include "app_espnow.h"

//------------------------------------------------------------------------------
// Defines

//------------------------------------------------------------------------------
// Values

static const char *TAG = "IO";
static const uint8_t btn_touch_id = 0;
static const uint8_t btn_func_id = 1;
static struct Button btn_touch;
static struct Button btn_func;

SemaphoreHandle_t btn_touch_single_click_sem = NULL;
SemaphoreHandle_t btn_touch_long_start_sem = NULL;
SemaphoreHandle_t btn_func_single_click_sem = NULL;
SemaphoreHandle_t btn_func_long_start_sem = NULL;

//------------------------------------------------------------------------------
//--------------------Function Defines--------------------

static uint8_t read_button_GPIO(uint8_t button_id)
{
    uint16_t touch_value;
    // you can share the GPIO read function with multiple Buttons
    switch (button_id)
    {
    case btn_touch_id:
#ifdef HT_NANO
        touch_pad_read_filtered(PIN_TOUCH, &touch_value);
        return (touch_value <= TOUCH_TRESHOULD ? 1 : 0);
#else
#if (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_LOW)
        return (!gpio_get_level(GPIO_CENTER_BUTTON));
#else
        return gpio_get_level(GPIO_CENTER_BUTTON);
#endif
#endif
    case btn_func_id:
#ifdef HT_NANO
#if (GPIO_OTA_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_LOW)
        return (!gpio_get_level(GPIO_OTA_BUTTON));
#else
        return gpio_get_level(GPIO_OTA_BUTTON);
#endif
#endif
        break;
    default:
        return 0;
    }
}

// center button
static void BTN_TOUCH_SINGLE_Click_Handler(void *btn)
{
    if (btn_touch_single_click_sem != NULL)
    {
        // Bee once when center.
        if (!isBinding())
        {
            buzzer_set_state(BUZZER_SINGLE, BUZZER_SINGLE_CLICK_MS, 0);
        }

        xSemaphoreGive(btn_touch_single_click_sem);
    }
}

static void BTN_TOUCH_LONG_PRESS_START_Handler(void *btn)
{
    if (btn_touch_long_start_sem != NULL)
    {
        // Bee long when center.
        if (!isBinding())
        {
            buzzer_set_state(BUZZER_SINGLE, BUZZER_SINGLE_CLICK_MS * 5, 0);
        }
        xSemaphoreGive(btn_touch_long_start_sem);
    }
}

// function button
static void BTN_FUNC_SINGLE_Click_Handler(void *btn)
{
}

static void BTN_FUNC_LONG_PRESS_START_Handler(void *btn)
{
    if (btn_func_long_start_sem != NULL)
    {
        xSemaphoreGive(btn_func_long_start_sem);
        set_binding_mode(true);
        buzzer_set_state(BUZZER_REPEAT, BUZZER_SINGLE_CLICK_MS, 1000);
    }
}

/**
 * @brief Did the center button single clicked?
 * @retval true: button was single clicked.
 * @retval false: button was NOT single clicked.
 */
bool isSingleClick(void)
{
    if (btn_touch_single_click_sem != NULL)
    {
        return xSemaphoreTake(btn_touch_single_click_sem, 0) == pdTRUE;
    }
    return false;
}

/**
 * @brief Is the center button in long start status?
 * @retval true: button is in long start status.
 * @retval false: button is NOT in long start status.
 */
bool isLongStart(void)
{
    if (btn_touch_long_start_sem != NULL)
    {
        return xSemaphoreTake(btn_touch_long_start_sem, 0) == pdTRUE;
    }
    return false;
}

void io_Thread(void *pvParameters)
{
    for (;;)
    {
        button_ticks(); // read button status
        buzzer_update(TICKS_INTERVAL);
        vTaskDelay(pdMS_TO_TICKS(TICKS_INTERVAL));
    }
}

void io_Init(void)
{

#ifdef HEADTRAKCER
    gpio_config_t io_conf = {};

    // config led io
    io_conf.intr_type = GPIO_INTR_DISABLE; // disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;       // set as output mode
    io_conf.pin_bit_mask = (1ULL << GPIO_LED_STATUS_SET);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // enable pull-up mode
    gpio_config(&io_conf);
    gpio_set_level(GPIO_LED_STATUS_SET, GPIO_LED_STATUS_SET_ACTIVE_LEVEL); // set led on
#ifdef HT_LITE
    // config center button io
    io_conf.intr_type = GPIO_INTR_DISABLE;                   // disable interrupt
    io_conf.mode = GPIO_MODE_INPUT;                          // set as input mode
    io_conf.pin_bit_mask = (1ULL << GPIO_CENTER_BUTTON_SET); // center button
#if (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_LOW)
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // enable pull-up mode
#elif (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_HIGH)
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; // enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#else
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#endif
    gpio_config(&io_conf);

    // config bluetooth led io
    io_conf.intr_type = GPIO_INTR_DISABLE; // disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;       // set as output mode
    io_conf.pin_bit_mask = (1ULL << GPIO_BT_STATUS_SET);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // enable pull-up mode
    gpio_config(&io_conf);
    gpio_set_level(GPIO_BT_STATUS_SET, !GPIO_BT_STATUS_SET_ACTIVE_LEVEL); // set led
#elif HT_NANO
    touch_Init();
    buzzer_init();

    // config ota button io
    io_conf.intr_type = GPIO_INTR_DISABLE;                // disable interrupt
    io_conf.mode = GPIO_MODE_INPUT;                       // set as input mode
    io_conf.pin_bit_mask = (1ULL << GPIO_OTA_BUTTON_SET); // OTA button(function button)
#if (GPIO_OTA_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_LOW)
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // enable pull-up mode
#elif (GPIO_OTA_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_HIGH)
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; // enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#else
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#endif
    gpio_config(&io_conf);
#endif
#endif
    // create semaphore if not already created
    //  center button
    if (btn_touch_single_click_sem == NULL)
    {
        btn_touch_single_click_sem = xSemaphoreCreateBinary();
        if (btn_touch_single_click_sem == NULL)
        {
            ESP_LOGW(TAG, "btn_touch_single_click_sem create FAILED");
            return;
        }
    }
    if (btn_touch_long_start_sem == NULL)
    {
        btn_touch_long_start_sem = xSemaphoreCreateBinary();
        if (btn_touch_long_start_sem == NULL)
        {
            ESP_LOGW(TAG, "btn_touch_long_start_sem create FAILED");
            return;
        }
    }
    // function button
    if (btn_func_single_click_sem == NULL)
    {
        btn_func_single_click_sem = xSemaphoreCreateBinary();
        if (btn_func_single_click_sem == NULL)
        {
            ESP_LOGW(TAG, "btn_func_single_click_sem create FAILED");
            return;
        }
    }
    if (btn_func_long_start_sem == NULL)
    {
        btn_func_long_start_sem = xSemaphoreCreateBinary();
        if (btn_func_long_start_sem == NULL)
        {
            ESP_LOGW(TAG, "btn_func_long_start_sem create FAILED");
            return;
        }
    }
    // initialize button function
    //  center button
    button_init(&btn_touch, read_button_GPIO, 1, btn_touch_id);
    button_attach(&btn_touch, SINGLE_CLICK, BTN_TOUCH_SINGLE_Click_Handler);
    button_attach(&btn_touch, LONG_PRESS_START, BTN_TOUCH_LONG_PRESS_START_Handler);
    button_start(&btn_touch);
    // function button
    button_init(&btn_func, read_button_GPIO, 1, btn_func_id);
    button_attach(&btn_func, SINGLE_CLICK, BTN_FUNC_SINGLE_Click_Handler);
    button_attach(&btn_func, LONG_PRESS_START, BTN_FUNC_LONG_PRESS_START_Handler);
    button_start(&btn_func);

    // create io task thread
    xTaskCreatePinnedToCore(io_Thread, "io_Thread", IO_THREAD_STACK_SIZE_SET, NULL, IO_THREAD_PRIORITY_SET, NULL, 1); // run on core1
}

#ifdef HT_LITE

/**
 * @brief set bt led status on or off
 * @param status 0 = off, 1 = on
 */
void led_bt_ctrl(uint8_t status)
{
    if (status == 1)
        gpio_set_level(GPIO_BT_STATUS_SET, GPIO_BT_STATUS_SET_ACTIVE_LEVEL); // set led off
    else
        gpio_set_level(GPIO_BT_STATUS_SET, !GPIO_BT_STATUS_SET_ACTIVE_LEVEL); // set led off
}

#endif
#endif