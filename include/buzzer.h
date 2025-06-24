#pragma once

#define BUZZER_SINGLE_CLICK_MS 100

// 蜂鸣器状态类型
typedef enum
{
    BUZZER_OFF,
    BUZZER_SINGLE,
    BUZZER_REPEAT
} buzzer_state_t;

// 蜂鸣器控制结构体
typedef struct
{
    buzzer_state_t state;     // 当前蜂鸣器状态
    uint32_t on_time_ms;      // 持续响的时间（毫秒）
    uint32_t off_time_ms;     // 间隔时间（毫秒）
    uint32_t elapsed_time_ms; // 已经过的时间（毫秒）
    bool is_on;               // 当前是否响着
} buzzer_t;

typedef struct
{
    uint32_t frequency; // Frequency of the tone in Hz
    uint32_t duration;  // Duration of the tone in milliseconds
} buzzer_tone_t;

typedef struct
{
    const buzzer_tone_t *tones; // Pointer to an array of tones
    size_t tone_count;          // Number of tones in the array
    size_t current_tone;        // Index of the current tone being played
    uint32_t elapsed_time_ms;   // Time elapsed for the current tone
} buzzer_tone_sequence_t;

extern buzzer_tone_t doremi[];
void buzzer_init(void);
void buzzer_update(uint32_t delta_time_ms);
void buzzer_play_tone_sequence(const buzzer_tone_t *tones, size_t tone_count);
void buzzer_set_state(buzzer_state_t state, uint32_t on_time_ms, uint32_t off_time_ms);