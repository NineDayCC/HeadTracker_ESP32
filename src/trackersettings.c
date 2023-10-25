#include "trackersettings.h"

TrackerSettings trkset = {
    .v.rollReverse = 0, // Roll direction reversed. Y:1 N:0
    .v.tiltReverse = 0, // Tilt direction reversed. Y:1 N:0
    .v.panReverse = 0,  // Pan direction reversed. Y:1 N:0

    .v.useMagn = 0,     // Use Magnetometer. Y:1 N:0
    .v.useBlutooth = 0, // Open Blutooth. Y:1 N:0

    .v.rollEn = 1, // Enable roll ppm output
    .v.tiltEn = 1, // Enable tile ppm output
    .v.panEn = 1,  // Enable pan ppm output

    .v.rollMax = DEF_MAX_PWM, // pwm max
    .v.rollMin = DEF_MIN_PWM, // pwm min
    .v.tiltMax = DEF_MAX_PWM, // pwm max
    .v.tiltMin = DEF_MIN_PWM, // pwm min
    .v.panMax = DEF_MAX_PWM,  // pwm max
    .v.panMin = DEF_MIN_PWM,  // pwm min

    .v.rollGain = 14.3, // 1000/servo max rang, 11 - 90degress, 14.3 - 70d
    .v.tiltGain = 14.3, // 1000/servo max rang, 11 - 90degress, 14.3 - 70d
    .v.panGain = 8.3, // 1000/servo max rang, 5.5 - 180degress, 8.3 -120d

    .v.rollCnt = PPM_CENTER, // Roll pwm center
    .v.tiltCnt = PPM_CENTER, // Tilt pwm center
    .v.panCnt = PPM_CENTER,  // Pan pwm center

    .v.rollChl = 6, // Roll channel
    .v.tiltChl = 7, // Tilt channel
    .v.panChl = 8,  // Pan channel

    .v.accOffset = {0.0f, 0.0f, 0.0f}, // in g
    .v.gyrOffset = {0.1659f, 0.5922f, 0.4395f}, // in degress
    // .v.gyrOffset = {0.1192821793f, 0.512078988f, 0.4421201354f}, // in degress
    // .v.gyrOffset = {0.0f, 0.0f, 0.0f}, // in degress

    .v.ppmframe = 20000, // PPM Frame Length (us)
    .v.ppmsync = 300,    // PPM Sync Pulse Length (us)
    .v.ppmchcnt = 8,     // PPM channels to output

    .v.btmode = 1,      //0:off 1:output mode
};
