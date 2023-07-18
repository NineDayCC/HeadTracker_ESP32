#ifndef TRACKERSETTINGS_H
#define TRACKERSETTINGS_H

#include <zephyr/kernel.h>

typedef union
{
    uint8_t array[80];

    struct __attribute__((packed))
    {
        uint8_t rollReverse : 1; // Roll direction reversed. Y:1 N:0
        uint8_t tiltReverse : 1; // Tilt direction reversed. Y:1 N:0
        uint8_t panReverse : 1;  // Pan direction reversed. Y:1 N:0

        uint8_t useMagn : 1;     // Use Magnetometer. Y:1 N:0
        uint8_t useBlutooth : 1; // Open Blutooth. Y:1 N:0

        uint8_t rollEn : 1; // Enable roll ppm output
        uint8_t tiltEn : 1; // Enable tilt ppm output
        uint8_t panEn : 1;  // Enable pan ppm output

        uint16_t rollMax; // pwm max
        uint16_t rollMin; // pwm min
        uint16_t tiltMax; // pwm max
        uint16_t tiltMin; // pwm min
        uint16_t panMax;  // pwm max
        uint16_t panMin;  // pwm min

        float rollGain;
        float tiltGain;
        float panGain;

        uint16_t rollCnt; // Roll pwm center
        uint16_t tiltCnt; // tilt pwm center
        uint16_t panCnt;  // pan pwm center

        uint8_t rollChl; // Roll channel
        uint8_t tiltChl; // Tilt channel
        uint8_t panChl;  // Pan channel

        float accOffset[3]; // accelerometerOffset in g, X Y Z
        float gyrOffset[3]; // gyroscopeOffset in degrees/s, X Y Z

        uint8_t ppmininvert : 1;
        uint8_t reserved : 7;

        uint16_t ppmframe; // PPM Frame Length (us)
        uint16_t ppmsync;  // PPM Sync Pulse Length (us)
        uint8_t ppmchcnt;  // PPM channels to output

        uint8_t btmode; // Bluetooth mode 0:off 1:output mode
    } v;                // Value
} TrackerSettings;

extern TrackerSettings trkset;

inline bool isUsingMagn(void) { return trkset.v.useMagn; }
inline bool isUsingBlutooth(void) { return trkset.v.useBlutooth; }
inline bool isRollReversed(void) { return trkset.v.rollReverse; }
inline bool isTiltReversed(void) { return trkset.v.tiltReverse; }
inline bool isPanReversed(void) { return trkset.v.panReverse; }
inline bool isRollEn(void) { return trkset.v.rollEn; }
inline bool isTiltEn(void) { return trkset.v.tiltEn; }
inline bool isPanEn(void) { return trkset.v.panEn; }

inline uint16_t getRollMax(void) { return trkset.v.rollMax; }
inline uint16_t getRollMin(void) { return trkset.v.rollMin; }
inline uint16_t getTiltMax(void) { return trkset.v.tiltMax; }
inline uint16_t getTiltMin(void) { return trkset.v.tiltMin; }
inline uint16_t getPanMax(void) { return trkset.v.panMax; }
inline uint16_t getPanMin(void) { return trkset.v.panMin; }

inline float getRollGain(void) { return trkset.v.rollGain; }
inline float getTiltGain(void) { return trkset.v.tiltGain; }
inline float getPanGain(void) { return trkset.v.panGain; }

inline uint16_t getRollCnt(void) { return trkset.v.rollCnt; }
inline uint16_t getTiltCnt(void) { return trkset.v.tiltCnt; }
inline uint16_t getPanCnt(void) { return trkset.v.panCnt; }

inline uint8_t getRollChl(void) { return trkset.v.rollChl; }
inline uint8_t getTiltChl(void) { return trkset.v.tiltChl; }
inline uint8_t getPanChl(void) { return trkset.v.panChl; }

inline float getAccOffset(uint8_t xyz) { return trkset.v.accOffset[xyz]; }
inline float getGyrOffset(uint8_t xyz) { return trkset.v.gyrOffset[xyz]; }

inline bool isPPMininvert(void) { return trkset.v.ppmininvert; }
inline uint16_t getPPMframe(void) { return trkset.v.ppmframe; }
inline uint16_t getPPMsync(void) { return trkset.v.ppmsync; }
inline uint8_t getPPMchcnt(void) { return trkset.v.ppmchcnt; }

inline uint8_t getBtMode(void) { return trkset.v.btmode; }

#define FLOAT_MIN -1000000
#define FLOAT_MAX 1000000
#define MIN_PWM 980
#define MAX_PWM 2020
#define DEF_MIN_PWM 988
#define DEF_MAX_PWM 2012
#define PPM_CENTER 1500
#define MIN_GAIN 0.01
#define MAX_GAIN 35
#define MINMAX_RNG 487
#define MIN_TO_CENTER 25
#define MIN_CNT (((MAX_PWM - MIN_PWM) / 2) + MIN_PWM - MINMAX_RNG)
#define MAX_CNT (((MAX_PWM - MIN_PWM) / 2) + MIN_PWM + MINMAX_RNG)
#define BT_CHANNELS 8
#define MAX_CHANNELS 16
#define AUX_FUNCTIONS 7
#define SBUS_CENTER 992
#define SBUS_SCALE 1.6
#define RESET_ON_TILT_TIME 1.5
#define RESET_ON_TILT_AFTER 1
#define RECENTER_PULSE_DURATION 0.5
#define UART_ACTIVE_TIME 0.1
#define PPM_MIN_FRAMESYNC 3000
#define PPM_MIN_SYNC 500
#define PPM_MIN_FRAME 6666
#define PPM_MAX_FRAME 40000
#define UART_MODE_OFF 0
#define UART_MODE_SBUS 1
#define UART_MODE_CRSFIN 2
#define UART_MODE_CRSFOUT 3

#endif