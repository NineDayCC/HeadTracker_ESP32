#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "io.h"
#include "multi_button.h"
#include "touch.h"
#include "buzzer.h"

//------------------------------------------------------------------------------
// Defines


//------------------------------------------------------------------------------
// Values

static const char* IO_TAG = "IO";
static const uint8_t btn1_id = 0;
static struct Button btn1;

SemaphoreHandle_t btn1_single_click_sem = NULL;
SemaphoreHandle_t btn1_long_start_sem = NULL;

//------------------------------------------------------------------------------
//--------------------Function Defines--------------------

void io_Init(void)
{

#ifdef HEADTRAKCER
    gpio_config_t io_conf = {};

    //config led io
    io_conf.intr_type = GPIO_INTR_DISABLE;  //disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;         //set as output mode
    io_conf.pin_bit_mask = (1ULL<<GPIO_LED_STATUS_SET);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;       //enable pull-up mode
    gpio_config(&io_conf);
    gpio_set_level(GPIO_LED_STATUS_SET, GPIO_LED_STATUS_SET_ACTIVE_LEVEL); //set led on
#ifdef HT_LITE
    //config center button io
    io_conf.intr_type = GPIO_INTR_DISABLE;  //disable interrupt
    io_conf.mode = GPIO_MODE_INPUT;         //set as input mode
    io_conf.pin_bit_mask = (1ULL<<GPIO_CENTER_BUTTON_SET);  //center button
    #if (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == ACTIVE_LOW)
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;       //enable pull-up mode
    #elif (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == ACTIVE_HIGH)
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;   //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    #else
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    #endif
    gpio_config(&io_conf);

    //config bluetooth led io
    io_conf.intr_type = GPIO_INTR_DISABLE;  //disable interrupt
    io_conf.mode = GPIO_MODE_OUTPUT;         //set as output mode
    io_conf.pin_bit_mask = (1ULL<<GPIO_BT_STATUS_SET);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;       //enable pull-up mode
    gpio_config(&io_conf);
    gpio_set_level(GPIO_BT_STATUS_SET, !GPIO_BT_STATUS_SET_ACTIVE_LEVEL); //set led 
#elif HT_NANO
    touch_Init();
    buzzer_init();

    //config ota button io
    io_conf.intr_type = GPIO_INTR_DISABLE;  //disable interrupt
    io_conf.mode = GPIO_MODE_INPUT;         //set as input mode
    io_conf.pin_bit_mask = (1ULL<<GPIO_OTA_BUTTON_SET);  //OTA button
    #if (GPIO_OTA_BUTTON_ACTIVE_LEVEL == ACTIVE_LOW)
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;       //enable pull-up mode
    #elif (GPIO_OTA_BUTTON_ACTIVE_LEVEL == ACTIVE_HIGH)
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;   //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    #else
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    #endif
    gpio_config(&io_conf);
#endif
#endif
    //create semaphore if not already created
    if (btn1_single_click_sem == NULL)
    {
        btn1_single_click_sem = xSemaphoreCreateBinary();
        if (btn1_single_click_sem == NULL)
        {
            ESP_LOGW(IO_TAG, "btn1_single_click_sem create FAILED");
            return;
        }
    }
    if (btn1_long_start_sem == NULL)
    {
        btn1_long_start_sem = xSemaphoreCreateBinary();
        if (btn1_long_start_sem == NULL)
        {
            ESP_LOGW(IO_TAG, "btn1_long_start_sem create FAILED");
            return;
        }
    }
    //initialize button function
    button_init(&btn1, read_button_GPIO, 1, btn1_id);
    button_attach(&btn1, SINGLE_CLICK, BTN1_SINGLE_Click_Handler);
    button_attach(&btn1, LONG_PRESS_START, BTN1_LONG_PRESS_START_Handler);
    button_start(&btn1);

    //create io task thread
    xTaskCreatePinnedToCore(io_Thread, "io_Thread", IO_THREAD_STACK_SIZE_SET, NULL, IO_THREAD_PRIORITY_SET, NULL, 1);  // run on core1
}

uint8_t read_button_GPIO(uint8_t button_id)
{
    uint16_t touch_value;
    // you can share the GPIO read function with multiple Buttons
    switch (button_id)
    {
    case btn1_id:
        #ifdef HT_NANO
        touch_pad_read_filtered(PIN_TOUCH, &touch_value);
        return (touch_value <= TOUCH_TRESHOULD ? 1 : 0);
        #else
            #if (GPIO_CENTER_BUTTON_ACTIVE_LEVEL == ACTIVE_LOW)
            return (gpio_get_level(GPIO_CENTER_BUTTON) ? 0 : 1);
            #else
            return gpio_get_level(GPIO_CENTER_BUTTON);
            #endif
        #endif
    default:
        return 0;
    }
}

void BTN1_SINGLE_Click_Handler(void* btn)
{
    if (btn1_single_click_sem != NULL)
    {
        set_buzzer_ms(BUZZER_SINGLE_CLICK_MS, TICKS_INTERVAL);
        xSemaphoreGive(btn1_single_click_sem);
    }
}

void BTN1_LONG_PRESS_START_Handler(void* btn)
{
    if (btn1_long_start_sem != NULL)
    {
        xSemaphoreGive(btn1_long_start_sem);
    }
}

/**
 * @brief Did the center button single clicked?
 * @retval true: button was single clicked.
 * @retval false: button was NOT single clicked.
 */
bool isSingleClick(void)
{
    if (btn1_single_click_sem != NULL)
    {
        return xSemaphoreTake(btn1_single_click_sem, 0) == pdTRUE;
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
    if (btn1_long_start_sem != NULL)
    {
        return xSemaphoreTake(btn1_long_start_sem, 0) == pdTRUE;
    }
    return false;
}

void io_Thread(void *pvParameters)
{
    for (;;)
    {
        button_ticks(); // read button status
        buzzer_loop();
        vTaskDelay(pdMS_TO_TICKS(TICKS_INTERVAL));
    }
}


#ifdef HT_LITE

/**
 * @brief set bt led status on or off
 * @param status 0 = off, 1 = on
 */
void led_bt_ctrl(uint8_t status)
{
    if (status == 1)
        gpio_set_level(GPIO_BT_STATUS_SET, GPIO_BT_STATUS_SET_ACTIVE_LEVEL); //set led off
    else
        gpio_set_level(GPIO_BT_STATUS_SET, !GPIO_BT_STATUS_SET_ACTIVE_LEVEL); //set led off
}

#endif