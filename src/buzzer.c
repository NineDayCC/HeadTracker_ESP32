#if defined HT_NANO || defined HT_NANO_V2
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "buzzer.h"
#include "ht.h"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO (GPIO_BUZZER) // Define the output GPIO
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (4096)                // Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY (4000)           // Frequency in Hertz. Set frequency at 4 kHz

static buzzer_t buzzer = {
    .state = BUZZER_OFF,
    .on_time_ms = 0,
    .off_time_ms = 0,
    .elapsed_time_ms = 0,
    .is_on = false};

// Set the buzzer peripheral configuration
void buzzer_init(void)
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
        .hpoint = 0};
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static inline void SET_BUZZER_ON(void)
{
    // Set duty
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
    // Update duty to apply the new value
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}
static inline void SET_BUZZER_OFF(void)
{
    // Set duty
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    // Update duty to apply the new value
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// 设置蜂鸣器状态
void buzzer_set_state(buzzer_state_t state, uint32_t on_time_ms, uint32_t off_time_ms)
{
    buzzer.state = state;             // 设置蜂鸣器状态
    buzzer.on_time_ms = on_time_ms;   // 设置响的时间
    buzzer.off_time_ms = off_time_ms; // 设置间隔时间
    buzzer.elapsed_time_ms = 0;       // 重置已过时间计数
    buzzer.is_on = false;             // 确保初始状态为关闭
    SET_BUZZER_OFF();                 // 设置引脚为低电平关闭蜂鸣器
}

void buzzer_update(uint32_t delta_time_ms)
{
    buzzer.elapsed_time_ms += delta_time_ms; // 累计已过时间

    switch (buzzer.state)
    {
    case BUZZER_OFF:
        SET_BUZZER_OFF(); // 确保蜂鸣器关闭
        buzzer.is_on = false;
        break;

    case BUZZER_SINGLE:
        if (buzzer.elapsed_time_ms < buzzer.on_time_ms)
        {
            SET_BUZZER_ON(); // 持续响
            buzzer.is_on = true;
        }
        else
        {
            SET_BUZZER_OFF(); // 超过设定时间后关闭
            buzzer.is_on = false;
            buzzer.state = BUZZER_OFF; // 单次响后自动切换为关闭状态
        }
        break;

    case BUZZER_REPEAT:
        if (buzzer.is_on && buzzer.elapsed_time_ms >= buzzer.on_time_ms)
        {
            SET_BUZZER_OFF(); // 结束响并关闭
            buzzer.is_on = false;
            buzzer.elapsed_time_ms = 0; // 重置时间计数
        }
        else if (!buzzer.is_on && buzzer.elapsed_time_ms >= buzzer.off_time_ms)
        {
            SET_BUZZER_ON(); // 开始响
            buzzer.is_on = true;
            buzzer.elapsed_time_ms = 0; // 重置时间计数
        }
        break;
    }
}

#endif