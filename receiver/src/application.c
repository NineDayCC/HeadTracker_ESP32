#include "application.h"
#include "defines.h"
#include "PPM.h"
#include "bt.h"

void start(void)
{
    PPMinit();
    bt_init();
}

K_THREAD_DEFINE(bt_Thread_id, 1024, bt_Thread, NULL, NULL, NULL, BT_THREAD_PRIO, 0, 5000);

const char *now_str(void)
{
    static char buf[24]; /* ...HH:MM:SS.MMM,UUU */
    uint64_t now = micros64();
    uint16_t us = now % USEC_PER_MSEC;
    uint16_t ms;
    uint8_t s;
    uint8_t min;
    uint32_t h;

    now /= USEC_PER_MSEC;
    ms = now % MSEC_PER_SEC;
    now /= MSEC_PER_SEC;
    s = now % 60U;
    now /= 60U;
    min = now % 60U;
    now /= 60U;
    h = now;

    snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u,%03u",
             h, min, s, ms, us);
    return buf;
}
