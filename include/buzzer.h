#pragma once
#include "driver/ledc.h"

#define BUZZER_SINGLE_CLICK_MS 100

void buzzer_init(void);
void buzzer_loop(void);
void set_buzzer_ms(uint32_t ms, uint32_t ticks_intervel);