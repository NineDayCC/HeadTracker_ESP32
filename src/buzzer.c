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

buzzer_tone_t doremi[] = {
    {1046, 500}, // C4
    {1175, 500}, // D4
    {1319, 500}, // E4
    {1397, 500}, // F4
    {1568, 500}, // G4
    {1760, 500}, // A4
    {1976, 500}, // B4
    {2093, 500}  // C5
};

static buzzer_tone_sequence_t tone_sequence = {0};

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
    // update flag
    buzzer.is_on = true;
}

static inline void SET_BUZZER_OFF(void)
{
    // Set duty
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    // Update duty to apply the new value
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    // update flag
    buzzer.is_on = false;
}

/**
 * @brief Set the buzzer state.
 * @param state: The state of the buzzer (BUZZER_OFF, BUZZER_SINGLE, BUZZER_REPEAT).
 * @param on_time_ms: The time in milliseconds for the buzzer to be ON.
 * @param off_time_ms: The time in milliseconds for the buzzer to be OFF.
 */
void buzzer_set_state(buzzer_state_t state, uint32_t on_time_ms, uint32_t off_time_ms)
{
    buzzer.state = state;             // 设置蜂鸣器状态
    buzzer.on_time_ms = on_time_ms;   // 设置响的时间
    buzzer.off_time_ms = off_time_ms; // 设置间隔时间
    buzzer.elapsed_time_ms = 0;       // 重置已过时间计数
    SET_BUZZER_OFF();                 // 设置引脚为低电平关闭蜂鸣器
}

/**
 * @brief Play a sequence of tones.
 * @param tones: Pointer to an array of buzzer_tone_t structures.
 * @param tone_count: Number of tones in the array.
 */
void buzzer_play_tone_sequence(const buzzer_tone_t *tones, size_t tone_count)
{
    tone_sequence.tones = tones;
    tone_sequence.tone_count = tone_count;
    tone_sequence.current_tone = 0;
    tone_sequence.elapsed_time_ms = 0;

    if (tone_count > 0)
    {
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, tones[0].frequency); // Set the frequency of the first tone
        SET_BUZZER_ON();
    }
}

/**
 * @brief Update the buzzer state and play the tone sequence if applicable. Place this function in the main loop.
 * @param delta_time_ms: Time elapsed since the last update in milliseconds.
 */
void buzzer_update(uint32_t delta_time_ms)
{
    buzzer.elapsed_time_ms += delta_time_ms; // 累计已过时间

    if (tone_sequence.tones && tone_sequence.current_tone < tone_sequence.tone_count)
    {
        tone_sequence.elapsed_time_ms += delta_time_ms;

        if (tone_sequence.elapsed_time_ms >= tone_sequence.tones[tone_sequence.current_tone].duration)
        {
            tone_sequence.elapsed_time_ms = 0;
            tone_sequence.current_tone++;

            if (tone_sequence.current_tone < tone_sequence.tone_count)
            {
                ledc_set_freq(LEDC_MODE, LEDC_TIMER, tone_sequence.tones[tone_sequence.current_tone].frequency); // Set the next tone frequency
                SET_BUZZER_ON();
            }
            else
            {
                SET_BUZZER_OFF();           // End of sequence
                tone_sequence.tones = NULL; // Reset the sequence
                tone_sequence.tone_count = 0;
                tone_sequence.current_tone = 0;
                tone_sequence.elapsed_time_ms = 0; // Reset elapsed time
                buzzer.state = BUZZER_OFF;         // Reset the state
                buzzer.elapsed_time_ms = 0;        // Reset elapsed time
            }
        }
        return;
    }

    ledc_set_freq(LEDC_MODE, LEDC_TIMER, LEDC_FREQUENCY); // Reset frequency to default
    switch (buzzer.state)
    {
    case BUZZER_OFF:
        SET_BUZZER_OFF(); // 确保蜂鸣器关闭
        break;

    case BUZZER_SINGLE:
        if (buzzer.elapsed_time_ms < buzzer.on_time_ms)
        {
            SET_BUZZER_ON(); // 持续响
        }
        else
        {
            SET_BUZZER_OFF();          // 超过设定时间后关闭
            buzzer.state = BUZZER_OFF; // 单次响后自动切换为关闭状态
        }
        break;

    case BUZZER_REPEAT:
        if (buzzer.is_on && buzzer.elapsed_time_ms >= buzzer.on_time_ms)
        {
            SET_BUZZER_OFF();           // 结束响并关闭
            buzzer.elapsed_time_ms = 0; // 重置时间计数
        }
        else if (!buzzer.is_on && buzzer.elapsed_time_ms >= buzzer.off_time_ms)
        {
            SET_BUZZER_ON();            // 开始响
            buzzer.elapsed_time_ms = 0; // 重置时间计数
        }
        break;
    }
}

#endif