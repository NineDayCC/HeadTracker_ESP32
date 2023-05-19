#include "trackersettings.h"

#include <zephyr/kernel.h>

TrackerSettings trkset = {
    .v.rollReverse = 0, // Roll direction reversed. Y:1 N:0
    .v.tiltReverse = 0, // Tilt direction reversed. Y:1 N:0
    .v.panReverse = 0,  // Pan direction reversed. Y:1 N:0

    .v.useMagn = 0,     // Use Magnetometer. Y:1 N:0
    .v.useBlutooth = 0, // Open Blutooth. Y:1 N:0

    .v.reserved0 = 0,
    .v.reserved1 = 0,
    .v.reserved2 = 0,

    .v.rollMax = DEF_MAX_PWM, // pwm max
    .v.rollMin = DEF_MIN_PWM, // pwm min
    .v.tiltMax = DEF_MAX_PWM, // pwm max
    .v.tiltMin = DEF_MIN_PWM, // pwm min
    .v.panMax = DEF_MAX_PWM,  // pwm max
    .v.panMin = DEF_MIN_PWM,  // pwm min

    .v.rollGain = 11, // 1000/servo max rang, 11 - 90degress
    .v.tiltGain = 11, // 1000/servo max rang, 11 - 90degress
    .v.panGain = 5.5, // 1000/servo max rang, 5.5 - 180degress

    .v.rollCnt = PPM_CENTER, // Roll pwm center
    .v.tiltCnt = PPM_CENTER, // Tilt pwm center
    .v.panCnt = PPM_CENTER,  // Pan pwm center

    .v.rollChl = 0, // Roll channel
    .v.tiltChl = 0, // Tilt channel
    .v.panChl = 0,  // Pan channel

    .v.accOffset = {0.0f, 0.0f, 0.0f},
    .v.gyrOffset = {0.2023113974f, -1.350461523f, -0.052654821f},

};
