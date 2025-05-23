#ifndef FRAMEWORK_ARDUINO

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "io.h"
#include "multi_button.h"
#include "touch.h"
#include "buzzer.h"
#include "app_espnow.h"
#include "ota.h"
#include "imu.h"
#include "led.h"


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
#ifdef HT_NANO
    uint16_t touch_value;
#endif

    // you can share the GPIO read function with multiple Buttons
    switch (button_id)
    {
    case btn_touch_id: // Touch pad
#ifdef HT_NANO
        touch_pad_read_filtered(PIN_TOUCH, &touch_value);
        return (touch_value <= TOUCH_TRESHOULD ? 1 : 0);
#endif

#if defined HT_NANO_V2 || defined HT_SE
#if (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_LOW)
        return (!gpio_get_level(GPIO_CENTER_BUTTON));
#else
        return gpio_get_level(GPIO_CENTER_BUTTON);
#endif
#endif

    case btn_func_id: // OTA button
#if defined HT_NANO || defined HT_NANO_V2
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
    return 0;
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

#if defined HT_NANO || defined HT_NANO_V2
// function button
static void BTN_FUNC_SINGLE_Click_Handler(void *btn)
{
    if (btn_func_single_click_sem != NULL && !isBinding() && !get_OTA_Mode())
    {
        xSemaphoreGive(btn_func_single_click_sem);
    }
}

static void BTN_FUNC_LONG_PRESS_START_Handler(void *btn)
{
    if (btn_func_long_start_sem != NULL && !get_OTA_Mode())
    {
        xSemaphoreGive(btn_func_long_start_sem);
        set_binding_mode(true);
        led_set_status(binding);
        buzzer_set_state(BUZZER_REPEAT, BUZZER_SINGLE_CLICK_MS, 1000);
    }
}
#endif

#if defined HT_NANO || defined HT_NANO_V2
// Detect OTA button
static void OTA_detect(void)
{
    static uint8_t cnt = 0;
    static uint32_t last_time = 0;
    if (xSemaphoreTake(btn_func_single_click_sem, 0))
    {
        if (cnt == 0)
        {
            last_time = millis64(); // get time at the first click
            cnt++;
        }
        else
        {
            if (millis64() - last_time < 500)
            {
                // if the second click is within 500ms
                // then enter OTA mode
                if (get_OTA_Mode() == false)
                {
                    ESP_LOGI(TAG, "OTA Mode");
                    imu_Deinit(); // Delet IMU task and calculation task
                    ht_espnow_deinit();
                    HttpOTA_server_init();                // OTA server init
                    buzzer_play_tone_sequence(doremi, 8); // play a tone sequence
                    set_OTA_Mode(true);
                    led_set_status(ota);
                }
            }
            else
            {
                cnt = 0;
            }
        }
    }
}
#endif

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
        led_update();
        #if defined HT_NANO || defined HT_NANO_V2
        OTA_detect(); // check if OTA mode
        #endif
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
#if defined HT_NANO_V2 || defined HT_SE
    // config center button io
    io_conf.intr_type = GPIO_INTR_DISABLE;                   // disable interrupt
    io_conf.mode = GPIO_MODE_INPUT;                          // set as input mode
    io_conf.pin_bit_mask = (1ULL << GPIO_CENTER_BUTTON_SET); // center button
#if (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_LOW)
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE; // enable pull-up mode
#elif (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == IO_ACTIVE_HIGH)
    io_conf.pull_down_en = GPIO_PULLUP_DISABLE; // enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#else
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
#endif
    gpio_config(&io_conf);
#endif

    touch_Init();
    buzzer_init();

#if defined HT_NANO || defined HT_NANO_V2

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
#if defined HT_NANO || defined HT_NANO_V2
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
#endif
    // initialize button function
    //  center button
    button_init(&btn_touch, read_button_GPIO, 1, btn_touch_id);
    button_attach(&btn_touch, SINGLE_CLICK, BTN_TOUCH_SINGLE_Click_Handler);
    button_attach(&btn_touch, LONG_PRESS_START, BTN_TOUCH_LONG_PRESS_START_Handler);
    button_start(&btn_touch);
#if defined HT_NANO || defined HT_NANO_V2
    // function button
    button_init(&btn_func, read_button_GPIO, 1, btn_func_id);
    button_attach(&btn_func, SINGLE_CLICK, BTN_FUNC_SINGLE_Click_Handler);
    button_attach(&btn_func, LONG_PRESS_START, BTN_FUNC_LONG_PRESS_START_Handler);
    button_start(&btn_func);
#endif

// create io task thread
#ifdef HT_NANO
    xTaskCreatePinnedToCore(io_Thread, "io_Thread", IO_THREAD_STACK_SIZE_SET, NULL, IO_THREAD_PRIORITY_SET, NULL, 1); // run on core1
#elif defined HT_NANO_V2 || defined HT_SE
    xTaskCreate(io_Thread, "io_Thread", IO_THREAD_STACK_SIZE_SET, NULL, IO_THREAD_PRIORITY_SET, NULL); // run on core0
#endif
}


#endif