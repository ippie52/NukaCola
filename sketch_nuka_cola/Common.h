/**
 * @file    Common.h
 *
 * @brief   Provides common functions, structures and other useful values.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#pragma once

/// @brief  The list of pins available
/// @TODO   Map these to different Arduino boards at some point
enum Pins
{
    // Outputs - These are all PWM capable
    DisplayLED1 = 3,
    DisplayLED2 = 5,
    DisplayLED3 = 6,
    DisplayLED4 = 9,
    DisplayLED5 = 10,
    DisplayLED6 = 11,

    // Settings adjustment indicators
    ModeLED = 2,
    SpeedLED  = 4,
    BrightnessLED = 7,

    // Inputs
    SettingSelectionBtn = 8,
    SettingDownBtn = 12,
    SettingUpBtn = 13

};


