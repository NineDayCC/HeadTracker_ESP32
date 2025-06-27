#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "cJSON.h"

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

typedef union
{
    uint8_t array[80];

    struct __attribute__((packed))
    {
        bool rollReverse; // Roll direction reversed. Y:1 N:0
        bool tiltReverse; // Tilt direction reversed. Y:1 N:0
        bool panReverse;  // Pan direction reversed. Y:1 N:0

        bool useMagn;     // Use Magnetometer. Y:1 N:0
        bool useBlutooth; // Open Blutooth. Y:1 N:0

        bool rollEn; // Enable roll ppm output
        bool tiltEn; // Enable tilt ppm output
        bool panEn;  // Enable pan ppm output

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

        bool ppmininvert;
        uint16_t ppmframe; // PPM Frame Length (us)
        uint16_t ppmsync;  // PPM Sync Pulse Length (us)
        uint8_t ppmchcnt;  // PPM channels to output

        bool btmode; // Bluetooth mode 0:off 1:output mode
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

inline uint8_t getRollChl(void) { return trkset.v.rollChl-1; }
inline uint8_t getTiltChl(void) { return trkset.v.tiltChl-1; }
inline uint8_t getPanChl(void) { return trkset.v.panChl-1; }

inline float getAccOffset(uint8_t xyz) { return trkset.v.accOffset[xyz]; }
inline float getGyrOffset(uint8_t xyz) { return trkset.v.gyrOffset[xyz]; }

inline bool isPPMininvert(void) { return trkset.v.ppmininvert; }
inline uint16_t getPPMframe(void) { return trkset.v.ppmframe; }
inline uint16_t getPPMsync(void) { return trkset.v.ppmsync; }
inline uint8_t getPPMchcnt(void) { return trkset.v.ppmchcnt; }

inline uint8_t getBtMode(void) { return trkset.v.btmode; }

bool setRll_Max(uint16_t val);
bool setRll_Min(uint16_t val);
bool setTiltMax(uint16_t val);
bool setTiltMin(uint16_t val);
bool setPanMax(uint16_t val);
bool setPanMin(uint16_t val);

bool setRollGain(uint16_t val);
bool setTiltGain(uint16_t val);
bool setPanGain(uint16_t val);

bool setRollCnt(uint16_t val);
bool setTiltCnt(uint16_t val);
bool setPanCnt(uint16_t val);

bool setRollChl(uint16_t val);
bool setTiltChl(uint16_t val);
bool setPanChl(uint16_t val);

bool setAccOffset(uint16_t val);
bool setGyrOffset(uint16_t val);

bool setPPMframe(uint16_t val);
bool setPPMsync(uint16_t val);
bool setPPMchcnt(uint16_t val);

bool setBtMode(uint16_t val);

cJSON *nvs_to_json();
void json_to_nvs(cJSON *json);
void trkset_init();
void trkset_restore_defaults();