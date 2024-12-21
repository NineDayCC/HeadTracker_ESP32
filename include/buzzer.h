#pragma once
#include "driver/ledc.h"

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

void buzzer_init(void);
void buzzer_update(uint32_t delta_time_ms);
void buzzer_set_state(buzzer_state_t state, uint32_t on_time_ms, uint32_t off_time_ms);