/**
 * @file    LedCluster.h
 *
 * @brief   Provides the LedCluster class, which can be used to draw patterns
 *          with a given collection of LEDs.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#pragma once
#include <string.h>
#include "NonVol.h"

class LedCluster;


enum Patterns
{
    ChaseClockwise,
    ChaseAntiClockwise,
    ChaseBoth,
    WaveClockwise,
    WaveAntiClockwise,
    Throb,
    Raindrop,
    Flames,
    Static,

    PATTERN_COUNT
};


struct Settings
{
    Patterns pattern;
    int brightnessMultiplier;
    int revsPerMinute;
    byte invalid;
};

struct LightLocationInfo
{
    float angle;
    long revolution;

};

struct LedInfo
{
    int index;
    float angle;
    int brightness;
    int pin;
    int extra;
};

typedef void (LedCluster::*PatternMethod)
(
    LedInfo * const,
    const LightLocationInfo * const
);

/// @brief  This look up table provides the brightness as a whole percentage
///         to the equivalent 8-bit duty cycle. As apparent brightness is more
///         logarithmic than linear, the values here show a logarithmic
///         increase. It's also worth noting that there are 101 values, from
///         zero to one hundred inclusive.
static const byte BRIGHTNESS_TO_DUTY_CYCLE[] =
{
   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
  10,  12,  13,  14,  15,  16,  17,  18,  20,  21,
  22,  23,  24,  26,  27,  28,  30,  31,  32,  33,
  35,  36,  38,  39,  40,  42,  43,  45,  46,  48,
  49,  51,  53,  54,  56,  57,  59,  61,  63,  64,
  66,  68,  70,  72,  74,  76,  78,  80,  82,  84,
  86,  88,  90,  93,  95,  97, 100, 102, 105, 107,
 110, 113, 116, 118, 121, 124, 128, 131, 134, 137,
 141, 145, 148, 152, 156, 160, 165, 169, 174, 179,
 184, 189, 195, 201, 207, 214, 221, 229, 237, 245, 255
};

enum BrightnessConstants
{
    MIN_BRIGHTNESS = 1,
    MAX_BRIGHTNESS = 20,
    BRIGHTNESS_DIVIDER = MAX_BRIGHTNESS,
    DEFAULT_BRIGHTNESS = 18
};

enum SpeedConstants
{
    MIN_SPEED = 6,
    MAX_SPEED = 60,
    SPEED_STEP = 3,
    DEFAULT_SPEED = 18
};

enum RaindropConstants
{
    RAINDROP_ANGLE = 12,
    RAMPUP_ANGLE = 3,
    RAMPDOWN_ANGLE = RAINDROP_ANGLE - RAMPUP_ANGLE
};


/*******************************************************************************
 * @brief   The LedCluster class, used to set LED brightnesses to form different
 *          patterns.
 */
class LedCluster
{
public:

    /***************************************************************************
     * @brief   Constructor - Populates the list of LED pins
     */
    LedCluster(const byte * const pins, const int count)
    : leds(nullptr)
    , count(count)
    , angle(360.0f / count)
    , startTimeMs(millis())
    , settingsNV(0)
    , running(true)
    {
        leds = new LedInfo[count];
        for (int i = 0; i < count; ++i)
        {
            leds[i].index = i;
            leds[i].angle = angle * i;
            leds[i].brightness = 0;
            leds[i].pin = pins[i];
        }
        populateRaindrops();
        settings = settingsNV;
        if (settings.invalid)
        {
            settings.pattern = Patterns::ChaseClockwise;
            settings.brightnessMultiplier = BrightnessConstants::DEFAULT_BRIGHTNESS;
            settings.revsPerMinute = SpeedConstants::DEFAULT_SPEED;
            settings.invalid = 0;
            settingsNV = settings;
        }
        revTimePeriodMs = ((1000.0f * 60.0f) / settings.revsPerMinute);

        // Serial.print(String("Angle: ") + angle + "*");
        // Serial.print(String("Time period: ") + revTimePeriodMs + "ms");
        /// @TODO  Get the current mode etc from flash/eeprom
    }

    /***************************************************************************
     * @brief   Destructor.
     */
    virtual ~LedCluster()
    {
        delete[] leds;
    }

    /***************************************************************************
     * @brief   Poll function, to be run once per loop operation. This takes the
     *          current run time and calculates where each light should be.
     */
    void poll()
    {
        static long lastRevolution = 0;
        if (running)
        {
            LightLocationInfo info;
            getCurrentLightInfo(&info);
            PatternMethod method = nullptr;
            switch (settings.pattern)
            {
                case Patterns::ChaseClockwise:
                    method = &LedCluster::chaseModeCw;
                    break;

                case Patterns::ChaseAntiClockwise:
                    method = &LedCluster::chaseModeAcw;
                    break;

                case Patterns::ChaseBoth:
                    method = &LedCluster::chaseModeBoth;
                    break;

                case Patterns::WaveClockwise:
                    method = &LedCluster::waveModeCw;
                    break;

                case Patterns::WaveAntiClockwise:
                    method = &LedCluster::waveModeAcw;
                    break;

                case Patterns::Throb:
                    method = &LedCluster::throbMode;
                    break;

                case Patterns::Raindrop:
                    if (lastRevolution != info.revolution)
                    {
                        populateRaindrops();
                    }
                    method = &LedCluster::raindropMode;
                    break;

                case Patterns::Flames:
                    method = &LedCluster::candleMode;
                    break;
                case Patterns::Static:
                    break;
            }
            if (method != nullptr)
            {
                for (int i = 0; i < count; ++i)
                {
                    (this->*method)(&leds[i], &info);
                }
                updateBrightnesses();
            }
            lastRevolution = info.revolution;
        }
    }

    void populateRaindrops()
    {
        for(int i = 0; i < count; i++)
        {
            leds[i].extra = random(360 - RaindropConstants::RAINDROP_ANGLE);
        }
    }

    static int forceRange(const int value, const int min_val, const int max_val)
    {
        return min(max(value, min_val), max_val);
    }

    bool updateBrightness(const int delta)
    {
        settings = settingsNV;
        const int newValue = forceRange(
            settings.brightnessMultiplier + delta,
            BrightnessConstants::MIN_BRIGHTNESS,
            BrightnessConstants::MAX_BRIGHTNESS
        );
        const bool change = newValue != settings.brightnessMultiplier;
        if (change)
        {
            settings.brightnessMultiplier = newValue;
            settingsNV = settings;
            Serial.println(String("Brightness is now ") + settings.brightnessMultiplier);
        }
        return change;
    }

    bool updatePattern(const int delta)
    {
        settings = settingsNV;
        settings.pattern = (settings.pattern + Patterns::PATTERN_COUNT + delta) % Patterns::PATTERN_COUNT;
        settingsNV = settings;
        Serial.println(String("Pattern is now: ") + settings.pattern);
        return true;
    }

    bool updateSpeed(const int delta)
    {
        settings = settingsNV;
        const int newValue = forceRange(
            settings.revsPerMinute + (delta * SpeedConstants::SPEED_STEP),
            SpeedConstants::MIN_SPEED,
            SpeedConstants::MAX_SPEED
        );
        const bool change = newValue != settings.revsPerMinute;
        if (change)
        {
            settings.revsPerMinute = newValue;
            settingsNV = settings;
            revTimePeriodMs = ((1000.0f * 60.0f) / settings.revsPerMinute);
            Serial.println(String("Speed is now ") + settings.revsPerMinute);
        }
        return change;
    }

    void startUp()
    {
        startTimeMs = millis();
        running = true;
        poll();
        // updateBrightnesses();
    }

    void shutdown()
    {
        running = false;
        for (int i = 0; i < count; ++i)
        {
            analogWrite(leds[i].pin, 0);
        }
    }

    /***************************************************************************
     * @brief   Gets the duty cycle to obtain the desired brightness.
     *
     * @param   brightness  The brightness, as a percentage from 0% (off) to
     *          100%  (fully on).
     *
     * @return  The duty cycle value, from 0 to 255.
     */
    static byte brightnessToDutyCycle(int brightness)
    {
        brightness = max(min(brightness, 100), 0);
        return BRIGHTNESS_TO_DUTY_CYCLE[brightness];
    }


    // void doubleChaseMode(LedInfo * const led, const LightLocationInfo *const info)
    // {
    //     const int angle = min(getLedAngleDelta(led, info, true), getLedAngleDelta(led, info, false));
    //     const int brightness = round((100.0f * (360 - angle)) / 360.0f);
    //     // Serial.println(String("LED ") + led->index + " Angle " + led->angle + " -> " + angle + " from leader, " + brightness + " brightness");
    //     led->brightness = brightness;
    // }


    // static int getLedAngleDelta(const LedInfo * const led, const LightLocationInfo * const info, const bool cw)
    // {
    //     if (cw)
    //     {
    //         return (int)((info->angle + 360.0f) - led->angle) % 360;
    //     }
    //     else
    //     {
    //         return (int)((info->angle + 360.0f) + led->angle) % 360;
    //     }
    // }

private:

    // void handleSequence(const LightLocationInfo * const info)
    // {
    //     PatternMethod method = nullptr;
    //     switch (settings.seqMode)
    //     {
    //         case SequenceModes::Chase:
    //             method = &LedCluster::chaseMode;
    //             break;
    //         case SequenceModes::MexicanWave:
    //             method = &LedCluster::waveMode;
    //             break;
    //         case SequenceModes::Pop:
    //             break;
    //         case SequenceModes::BinaryCount:
    //             break;
    //         case SequenceModes::Random:
    //             break;
    //     }
    //     if (method != nullptr)
    //     {
    //         for (int i = 0; i < count; ++i)
    //         {
    //             (this->*method)(&leds[i], info);
    //             // chaseMode(&leds[i], &info);
    //             // doubleChaseMode(&leds[i], &info);
    //             // waveMode(&leds[i], &info);
    //         }
    //     }
    // }
    void getCurrentLightInfo(LightLocationInfo * const info)
    {

        // Get the elapsed time since the start of the sequences
        long elapsedMs = (millis() - startTimeMs);
        info->angle = (360.0f * (elapsedMs % (long)revTimePeriodMs)) / revTimePeriodMs;
        info->revolution = elapsedMs / (long)revTimePeriodMs;
    }

    void chaseModeAcw(LedInfo * const led, const LightLocationInfo *const info)
    {
        const int angle = (int)((info->angle + 360.0f) + led->angle) % 360;
        led->brightness = globaliseBrightness(round((100.0f * (360 - angle)) / 360.0f));
    }
    void chaseModeCw(LedInfo * const led, const LightLocationInfo *const info)
    {
        const int angle = (int)((info->angle + 360.0f) - led->angle) % 360;
        led->brightness = globaliseBrightness(round((100.0f * (360 - angle)) / 360.0f));
    }
    void chaseModeBoth(LedInfo * const led, const LightLocationInfo *const info)
    {
        const int angle = min(
            (int)((info->angle + 360.0f) - led->angle) % 360,
            (int)((info->angle + 360.0f) + led->angle) % 360
        );
        led->brightness = globaliseBrightness(round((100.0f * (360 - angle)) / 360.0f));
    }

    void raindropMode(LedInfo * const led, const LightLocationInfo *const info)
    {
        const int position = info->angle - led->extra;
        // const int rampUpEnd = led->raindrop + RaindropConstants::RAMPUP_ANGLE;
        // const int rampDownEnd = led->raindrop + RaindropConstants::RAINDROP_ANGLE;

        if (position >= 0 && position < RaindropConstants::RAMPUP_ANGLE)
        {
            // Ramp up
            led->brightness = globaliseBrightness(
                round((100.0f * position) / RaindropConstants::RAMPUP_ANGLE)
            );
            // Serial.println(String("Ramping up ") +  led->brightness);

        }
        else if ((position >= RaindropConstants::RAMPUP_ANGLE) &&
                (position < RaindropConstants::RAINDROP_ANGLE))
        {
            /// Ramp down
            led->brightness = globaliseBrightness(100 -
                round((100.0f * (position - RaindropConstants::RAMPUP_ANGLE)) /
                    RaindropConstants::RAMPDOWN_ANGLE)
            );
            // Serial.println(String("Ramping Down ") +  led->brightness);
        }
        else
        {
            led->brightness = 0;
            // Serial.println(String("No ramp at ") +  info->angle);
        }

        // if (info->angle < led->raindrop)
        // {
        //     // Not yet time
        // }
        // else if ((info->angle >= led->raindrop) && (info->angle < rampUpEnd))
        // {
        //     // Ramp up
        // }
        // else if (info->angle < rampDownEnd)
        // {
        //     // Ramp down
        // }
        // else
        // {
        //     // Raindrop has passed on
        // }
    }
    void candleMode(LedInfo * const led, const LightLocationInfo *const info)
    {
        led->extra = forceRange(led->extra + random(-4, 4), 0, 100);
        led->brightness = globaliseBrightness(led->extra);
        // Serial.println(String("LED ") + led->index + " is now " + led->brightness);
    }

    // void chaseMode(LedInfo * const led, const LightLocationInfo *const info)
    // {
    //     // We know from the info where the brightest point should be, then we need to know
    //     // what angle from this point every LED is at
    //     int angle = 0;
    //     switch (settings.direction)
    //     {
    //         case Directions::Clockwise:
    //             angle = (int)((info->angle + 360.0f) - led->angle) % 360;
    //             break;
    //         case Directions::Anticlockwise:
    //             angle = (int)((info->angle + 360.0f) + led->angle) % 360;
    //             break;
    //         case Directions::Both:
    //             angle = min(
    //                 (int)((info->angle + 360.0f) - led->angle) % 360,
    //                 (int)((info->angle + 360.0f) + led->angle) % 360
    //             );
    //             break;
    //     }
    //     led->brightness = globaliseBrightness(round((100.0f * (360 - angle)) / 360.0f));
    //     // const int angle = getLedAngleDelta(led, info, false);
    //     // const int brightness = round((100.0f * (360 - angle)) / 360.0f);
    //     // Serial.println(String("LED ") + led->index + " Angle " + led->angle + " -> " + angle + " from leader, " + brightness + " brightness");
    //     // led->brightness = brightness;
    // }

    int globaliseBrightness(int brightness)
    {
        settings = settingsNV;
        if (settings.brightnessMultiplier != BrightnessConstants::MAX_BRIGHTNESS)
        {
            brightness = round(
                (float)(settings.brightnessMultiplier * brightness) /
                BrightnessConstants::MAX_BRIGHTNESS
            );
        }

        return brightness;
    }

    void updateBrightnesses()
    {
        for (int i = 0; i < count; ++i)
        {
            analogWrite(leds[i].pin, brightnessToDutyCycle(leds[i].brightness));
        }
    }

    void waveModeCw(LedInfo *const led, const LightLocationInfo * const info)
    {
        int angle = (int)(info->angle - led->angle);
        angle = abs(((angle + 180) % 360) - 180);
        led->brightness = globaliseBrightness(round((100.0f * (180 - angle)) / 180.0f));
    }

    void waveModeAcw(LedInfo *const led, const LightLocationInfo * const info)
    {
        int angle = (int)(info->angle - led->angle);
        angle = abs((((360 - angle) + 180) % 360) - 180);
        led->brightness = globaliseBrightness(round((100.0f * (180 - angle)) / 180.0f));
    }

    void throbMode(LedInfo *const led, const LightLocationInfo * const info)
    {
        led->brightness = globaliseBrightness(round(abs(180.0f - info->angle)));
    }


    LedInfo *leds;
    const int count;
    const float angle;
    float revsPerMinute;
    long startTimeMs;
    long revTimePeriodMs;
    NonVol<Settings> settingsNV;
    Settings settings;
    int brightnessMultiplier;
    bool running;
};
