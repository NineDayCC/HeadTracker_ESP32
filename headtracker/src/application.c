#include "application.h"
#include "defines.h"
#include "imu.h"
#include "PPM.h"
#include "io.h"
#include "bt.h"
#include "uart.h"


void start(void)
{
    io_Init();
    PPMinit();
    uart_init();
    imu_Init();
    bt_init();
}

K_THREAD_DEFINE(imu_Thread_id, 2048, imu_Thread, NULL, NULL, NULL, IMU_THREAD_PRIO,
                0, 100);
K_THREAD_DEFINE(calculate_Thread_id, 2048, calculate_Thread, NULL, NULL, NULL,
                CALCULATE_THREAD_PRIO, 0, 100);
K_THREAD_DEFINE(io_Thread_id, 512, io_Thread, NULL, NULL, NULL, IO_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE(bt_Thread_id, 1024, bt_Thread, NULL, NULL, NULL, BT_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE(uart_Thread_id, 1024, uart_Thread, NULL, NULL, NULL, UARTTX_THREAD_PRIO, 0, 200);
