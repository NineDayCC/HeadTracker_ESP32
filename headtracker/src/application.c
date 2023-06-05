#include "application.h"
#include "defines.h"
#include "imu.h"
#include "io.h"

void start(void)
{
    io_Init();
    PPMinit();
    imu_Init();
}

K_THREAD_DEFINE(imu_Thread_id, 2048, imu_Thread, NULL, NULL, NULL, IMU_THREAD_PRIO,
                0, 0);
K_THREAD_DEFINE(calculate_Thread_id, 2048, calculate_Thread, NULL, NULL, NULL,
                CALCULATE_THREAD_PRIO, 0, 0);
K_THREAD_DEFINE(io_Thread_id, 512, io_Thread, NULL, NULL, NULL, IO_THREAD_PRIO, 0, 0);