#ifndef DEFINES_H
#define DEFINES_H

// Math
#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif
#define M_G 9.806650

#define SAMPLE_RATE (int)(1000000 / IMU_PERIOD) // IMU sample rate, replace this with actual sample rate(Hz)

// Thread Periods
#define IO_PERIOD 25 // (ms) IO Period (button reading)
#define BT_PERIOD 7500        // (us) Bluetooth update rate
// #define SERIAL_PERIOD 30       // (ms) Serial processing
// #define DATA_PERIOD 2          // Multiplier of Serial Period (Live Data Transmission Speed)
#define IMU_PERIOD 5000             // (us) Sensor Reads
#define CALCULATE_PERIOD IMU_PERIOD // (us) Channel Calculations
// #define UART_PERIOD 4000       // (us) Update rate of UART
// #define PWM_FREQUENCY 50       // (ms) PWM Period
// #define PAUSE_BEFORE_FLASH 60  // (ms) Time to pause all threads before Flash writing

// Thread Priority Definitions
#define PRIORITY_LOW 12
#define PRIORITY_MED 9
#define PRIORITY_HIGH 6
#define PRIORITY_RT 3

// Thread Periods, Negative values mean cannot be pre-empted
#define IO_THREAD_PRIO PRIORITY_LOW
// #define SERIAL_THREAD_PRIO PRIORITY_LOW
// #define DATA_THREAD_PRIO PRIORITY_LOW
#define BT_THREAD_PRIO -15
#define IMU_THREAD_PRIO PRIORITY_MED
#define CALCULATE_THREAD_PRIO PRIORITY_HIGH
// #define UARTRX_THREAD_PRIO PRIORITY_LOW - 2
// #define UARTTX_THREAD_PRIO PRIORITY_HIGH

#define millis64() k_cyc_to_ms_floor64(k_cycle_get_64())
#define micros64() k_cyc_to_us_floor64(k_cycle_get_64())

#endif