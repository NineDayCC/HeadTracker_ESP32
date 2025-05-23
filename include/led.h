#pragma once

typedef enum
{
    connected = 0,
    disconnected,
    binding,
    ota,
} led_status_t;

void led_set_status(led_status_t status);
void led_update(void);

