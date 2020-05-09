/**
 * @file    LedCluster.h
 *
 * @brief   Provides the LedCluster class, which can be used to draw patterns
 *          with a given collection of LEDs.
 *          When creating patterns, this class uses the concept of a circle, or
 *          more appropriately, the angle of a point orbiting around along the
 *          circle. The head or lead of the circle is the focal point. For most
 *          patterns, this will be the brightest point. Others simply use this
 *          as the amount of time within the current revolution.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#pragma once
#include <string.h>
#include "NonVol.h"

/**
 * Constants
 */

/// @brief  The settings version number.
#define VERSION     (1)

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

/**
 * Forward declarations.
 */
class LedCluster;

/**
 * Structures, enumerations and type definitions.
 */

/// @brief  Provides the available LED illumination patterns.
enum Patterns
{
    JustOn,
    // Chasing - The lead LED comes on at full brightness,
    // and dims as the head of the circle moves around
    ChaseClockwise,
    ChaseAntiClockwise,
    ChaseBoth,
    // The peak of the circle is brightest, with the area surrounding it
    // dimming gently to nothing at 180 degrees away
    WaveClockwise,
    WaveAntiClockwise,
    // The lights all throb from off to on
    Throb,
    // Another throb mode with some flicker
    Throb2,
    // Heart beat effect, two pulses per revolution
    Heartbeat,
    // Each light randomly lights up quickly then fades once per revolution
    Raindrop,
    // Rough attempt at a candle flicker
    Flames,
    // Fairly random flickering
    Static,

    PATTERN_COUNT
};

const String PATTERN_STRINGS[Patterns::PATTERN_COUNT] =
{
    "Just On",
    "Chase Clockwise",
    "Chase AntiClockwise",
    "Chase Both",
    "Wave Clockwise",
    "Wave AntiClockwise",
    "Throb",
    "Throb Two",
    "Heartbeat",
    "Raindrop",
    "Flames",
    "Static",
};

/// @brief  The settings object, stored in non-volatile memory so that the
///         settings persist between power cycles.
struct Settings
{
    // The version number - This changes if the settings layout change
    int version;
    // The last selected illumination pattern
    Patterns pattern;
    // Multiplier used to set the maximum brightness
    int brightnessMultiplier;
    // The speed of the illumination patterns determined by how many revolutions
    // per minute around the "circle".
    int revsPerMinute;
    // Unwritten EEPROM bytes are all 0xFF. This boolean value will help identify
    // when a fresh read is done. It will not protect against structure changes.
    byte invalid;
};

/// @brief  Information representing the current position of the "lead" point
///         of the circle during a revolution.
struct LightLocationInfo
{
    // The current angle in degrees
    float angle;
    // The revolution number, good for identifying when a new cycle has started
    long revolution;
};

/// @brief  Information about each LED, allowing different illumination patterns
///         to be carried out.
struct LedInfo
{
    int index;
    float angle;
    int brightness;
    int pin;
    int extra;
};

/// @brief  Type definition for an illumination pattern method.
typedef void (LedCluster::*PatternMethod)
(
    LedInfo * const,
    const LightLocationInfo * const
);

/// @brief  Constants required for calculating the current brightnesses of
///         LEDs. These will be used to add a range to the maximum brightness
///         of the LEDs, such that they can be turned down if needs be.
enum BrightnessConstants
{
    // The minimum brightness level
    MIN_BRIGHTNESS = 1,
    // Maximum brightness level
    MAX_BRIGHTNESS = 20,
    // The divider to be applied to the current brightness level
    BRIGHTNESS_DIVIDER = MAX_BRIGHTNESS,
    // The default brightness level on a clean upload
    DEFAULT_BRIGHTNESS = 18,
    // The minimum brightness percentage
    MIN_BRIGHTNESS_PCT = ((100 * MIN_BRIGHTNESS) / MAX_BRIGHTNESS),
    // The maximum brightness percentage
    MAX_BRIGHTNESS_PCT = 100,

};

/// @brief  Constants required for calculating the speed of the illumination
///         pattern changes.
enum SpeedConstants
{
    // Minimum speed level (in revolutions per minute)
    MIN_SPEED = 6,
    // Maximum speed level (in revolutions per minute)
    MAX_SPEED = 60,
    // The number of speed levels to increase by per step
    SPEED_STEP = 3,
    // The default speed on a clean upload
    DEFAULT_SPEED = 18,
    // The minimum speed percentage
    MIN_SPEED_PCT = ((100 * MIN_SPEED) / MAX_SPEED),
    // The maximum speed percentage
    MAX_SPEED_PCT = 100,
};

/// @brief  Constants required to create a raindrop effect.
enum RaindropConstants
{
    // The number of degrees the raindrop will appear over
    RAINDROP_ANGLE = 12,
    // The number of degrees the raindrop will take to brighten
    RAMPUP_ANGLE = 3,
    // The number of degrees the raindrop will fade over
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
     *
     * @param   pins    Pointer to the collection of pin IDs
     * @param   count   The number of LEDs in this cluster
     */
    LedCluster(const byte * const pins, const int count)
    : leds(nullptr)
    , count(count)
    , startTimeMs(millis())
    , settingsNV(0)
    , running(true)
    {
        // Set up the LEDs
        const float angle = 360.0f / count;
        leds = new LedInfo[count];
        for (int i = 0; i < count; ++i)
        {
            leds[i].index = i;
            leds[i].angle = angle * i;
            leds[i].brightness = 0;
            leds[i].pin = pins[i];
        }
        populateRaindrops();
        // Load the settings and check they are valid, set to defaults if not
        settings = settingsNV;
        if (settings.invalid || settings.version != VERSION)
        {
            settings.version = VERSION;
            settings.pattern = Patterns::ChaseClockwise;
            settings.brightnessMultiplier = BrightnessConstants::DEFAULT_BRIGHTNESS;
            settings.revsPerMinute = SpeedConstants::DEFAULT_SPEED;
            settings.invalid = 0;
            settingsNV = settings;
        }
        // Calculate the current time period of the illumination pattern
        revTimePeriodMs = ((1000.0f * 60.0f) / settings.revsPerMinute);
    }

    /***************************************************************************
     * @brief   Destructor.
     */
    virtual ~LedCluster()
    {
        delete[] leds;
    }

    /***************************************************************************
     * @brief   Method used to ensure that a value is within its minimum and
     *          maximum range values.
     *
     * @param   value   The value to test
     * @param   min_val The minimum allowed value
     * @param   max_val The maximum allowed value
     *
     * @return  The capped value once passed through the range criteria.
     */
    static int forceRange(const int value, const int min_val, const int max_val)
    {
        return min(max(value, min_val), max_val);
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
        brightness = forceRange(brightness, 0, 100);
        return BRIGHTNESS_TO_DUTY_CYCLE[brightness];
    }


    /***************************************************************************
     * @brief   Poll function, to be run once per loop operation. This takes the
     *          current run time and calculates what the illumination levels of
     *          each LED should be based on their position, the pattern and
     *          other factors.
     */
    void poll()
    {
        static long lastRevolution = 0;
        if (running)
        {
            LightLocationInfo info;
            getCurrentLightInfo(&info);
            PatternMethod method = nullptr;
            // Identify the required pattern method. Whilst this could be
            // pushed into an array, that would require additional handling
            // for invalid indices, and special conditions for patterns that
            // require additional functions to be carried out on occasion.
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

                case Patterns::Throb2:
                    method = &LedCluster::throbMode2;
                    break;

                case Patterns::Heartbeat:
                    method = &LedCluster::heartbeatMode;
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
                    method = &LedCluster::staticMode;
                    break;

                case Patterns::JustOn:
                    method = &LedCluster::justOn;
                default:
                    // No point doing anything, printing to the serial port
                    // would flood it.
                    break;
            }
            if (method != nullptr)
            {
                for (int i = 0; i < count; ++i)
                {
                    (this->*method)(&leds[i], &info);
                }
                updateLedBrightnesses();
            }
            lastRevolution = info.revolution;
        }
        // Small delay, allowing the LED PWM values to settle
        delay(20);
    }

    bool setBrightnessPercent(const int percent)
    {
        const int value = (percent * BrightnessConstants::MAX_BRIGHTNESS) / 100;
        return setBrightness(value);
    }

    bool setBrightness(const int value)
    {
        settings = settingsNV;
        const int newValue = forceRange(
            value,
            BrightnessConstants::MIN_BRIGHTNESS,
            BrightnessConstants::MAX_BRIGHTNESS
        );
        const bool change = newValue != settings.brightnessMultiplier;
        if (change)
        {
            settings.brightnessMultiplier = newValue;
            settingsNV = settings;
        }
        if (Serial)
        {
            const int percent = (100 * newValue) / BrightnessConstants::MAX_BRIGHTNESS;
            Serial.println(String("Brightness now at ") + percent + "%");
        }
        return change;
    }

    /***************************************************************************
     * @brief   Updates the current global brightness level with a change to
     *          the current level.
     *
     * @param   delta     The change in value to be applied to the brightnesses
     *
     * @return  Success of the action, true if the brightness was updated
     */
    bool updateBrightness(const int delta)
    {
        settings = settingsNV;
        return setBrightness(settings.brightnessMultiplier + delta);
        // const int newValue = forceRange(
        //     settings.brightnessMultiplier + delta,
        //     BrightnessConstants::MIN_BRIGHTNESS,
        //     BrightnessConstants::MAX_BRIGHTNESS
        // );
        // const bool change = newValue != settings.brightnessMultiplier;
        // if (change)
        // {
        //     settings.brightnessMultiplier = newValue;
        //     settingsNV = settings;
        // }
        // if (Serial)
        // {
        //     const int percent = (100 * newValue) / BrightnessConstants::MAX_BRIGHTNESS;
        //     Serial.println(String("Brightness now at ") + percent + "%");
        // }
        // return change;
    }

    bool setPattern(const int pattern)
    {
        settings = settingsNV;
        const int newValue = forceRange(pattern, 0, Patterns::PATTERN_COUNT);
        const bool change = settings.pattern != newValue;
        if (change)
        {
            settings.pattern = newValue;
            settingsNV = settings;
        }
        if (Serial)
        {
            Serial.println(
                String("Illumination pattern is now \"") +
                PATTERN_STRINGS[settings.pattern] + "\""
            );
        }
        return change;
    }


    /***************************************************************************
     * @brief   Updates the current illumination pattern, by incrementing or
     *          decrementing the selected index by the given amound.
     *
     * @param   delta  The value to be added to the current pattern selection
     *
     * @return  Success of the action, true if the pattern was updated
     */
    bool updatePattern(const int delta)
    {
        settings = settingsNV;
        const int pattern = (settings.pattern + Patterns::PATTERN_COUNT + delta) % Patterns::PATTERN_COUNT;
        return setPattern(pattern);
    }

    /***************************************************************************
     * @brief   Updates the current speed by incrementing or decrementing by
     *          the given amount, multiplied by the speed step change.
     *
     * @param   delta  The number of steps to be added to the current speed
     *
     * @return  Success of the action, true if the speed was updated
     */
    bool updateSpeed(const int delta)
    {
        settings = settingsNV;
        return setSpeed(settings.revsPerMinute + (delta * SpeedConstants::SPEED_STEP));
    }

    bool setSpeedPercent(const int percent)
    {
        const int value = (SpeedConstants::MAX_SPEED * percent) / 100;
        return setSpeed(value);
    }

    bool setSpeed(const int speed)
    {
        settings = settingsNV;
        const int newValue = forceRange(
            speed,
            SpeedConstants::MIN_SPEED,
            SpeedConstants::MAX_SPEED
        );
        const bool change = newValue != settings.revsPerMinute;
        if (change)
        {
            settings.revsPerMinute = newValue;
            settingsNV = settings;
            revTimePeriodMs = ((1000.0f * 60.0f) / settings.revsPerMinute);
        }
        if (Serial)
        {
            const int percent = (100 * newValue) / SpeedConstants::MAX_SPEED;
            Serial.println(String("Speed now at ") + percent + "%");
        }
        return change;
    }


    /***************************************************************************
     * @brief   Starts the illumination pattern when not in the running state.
     */
    void startUp()
    {
        if (!running)
        {
            startTimeMs = millis();
            running = true;
            poll();
        }
    }

    /***************************************************************************
     * @brief   Stops running mode, and turns off the LEDs.
     */
    void shutdown()
    {
        running = false;
        for (int i = 0; i < count; ++i)
        {
            analogWrite(leds[i].pin, 0);
        }
    }

private:

    /***************************************************************************
     * @brief   Populates the values required for the raindrop illumination
     *          pattern.
     */
    void populateRaindrops()
    {
        for(int i = 0; i < count; i++)
        {
            leds[i].extra = random(360 - RaindropConstants::RAINDROP_ANGLE);
        }
    }

    /***************************************************************************
     * @brief   Gets the light information for where the "lead" point around
     *          the circle is, and how many revolutions have occurred. This is
     *          based on the current speed.
     *
     * @param   info    The LightLocationInfo pointer to populate
     */
    void getCurrentLightInfo(LightLocationInfo * const info)
    {

        // Get the elapsed time since the start of the sequences
        long elapsedMs = (millis() - startTimeMs);
        info->angle = (360.0f * (elapsedMs % (long)revTimePeriodMs)) / revTimePeriodMs;
        info->revolution = elapsedMs / (long)revTimePeriodMs;
    }

    /***************************************************************************
     * @brief   Lights are simply on at the global brightness
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void justOn(LedInfo * const led, const LightLocationInfo *const info)
    {
        led->brightness = globaliseBrightness(100);
    }

    /***************************************************************************
     * @brief   Chase mode (anti-clockwise).
     *          Chasing - The lead LED comes on at full brightness, and dims as
     *          the head of the circle moves around.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void chaseModeAcw(LedInfo * const led, const LightLocationInfo *const info)
    {
        const int angle = (int)((info->angle + 360.0f) + led->angle) % 360;
        led->brightness = globaliseBrightness(round((100.0f * (360 - angle)) / 360.0f));
    }

    /***************************************************************************
     * @brief   Chase mode (clockwise).
     *          Chasing - The lead LED comes on at full brightness, and dims as
     *          the head of the circle moves around.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void chaseModeCw(LedInfo * const led, const LightLocationInfo *const info)
    {
        const int angle = (int)((info->angle + 360.0f) - led->angle) % 360;
        led->brightness = globaliseBrightness(round((100.0f * (360 - angle)) / 360.0f));
    }

    /***************************************************************************
     * @brief   Chase mode (both).
     *          Chasing - The lead LED comes on at full brightness, and dims as
     *          the head of the circle moves around. With this case, the head of
     *          the circle is mirrored at 180 degrees.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void chaseModeBoth(LedInfo * const led, const LightLocationInfo *const info)
    {
        const int angle = min(
            (int)((info->angle + 360.0f) - led->angle) % 360,
            (int)((info->angle + 360.0f) + led->angle) % 360
        );
        led->brightness = globaliseBrightness(round((100.0f * (360 - angle)) / 360.0f));
    }

    /***************************************************************************
     * @brief   Raindrop mode.
     *          Each light randomly lights up quickly then fades once per
     *          revolution.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void raindropMode(LedInfo * const led, const LightLocationInfo *const info)
    {
        const int position = info->angle - led->extra;
        if (position >= 0 && position < RaindropConstants::RAMPUP_ANGLE)
        {
            // Ramp up
            led->brightness = globaliseBrightness(
                round((100.0f * position) / RaindropConstants::RAMPUP_ANGLE)
            );

        }
        else if ((position >= RaindropConstants::RAMPUP_ANGLE) &&
                (position < RaindropConstants::RAINDROP_ANGLE))
        {
            /// Ramp down
            led->brightness = globaliseBrightness(100 -
                round((100.0f * (position - RaindropConstants::RAMPUP_ANGLE)) /
                    RaindropConstants::RAMPDOWN_ANGLE)
            );
        }
        else
        {
            led->brightness = 0;
        }
    }

    /***************************************************************************
     * @brief   Candle (flame) mode.
     *          Rough attempt at a candle flicker. Currently this works well
     *          enough, though takes a little while to settle, and it's far from
     *          realistic.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void candleMode(LedInfo * const led, const LightLocationInfo *const info)
    {
        led->extra = forceRange(led->extra + random(-4, 4), 0, 100);
        led->brightness = globaliseBrightness(led->extra);
    }

    /***************************************************************************
     * @brief   Static noise mode.
     *          Fairly random flickering. This is based on candle mode, but with
     *          some additional spikes of noise added.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void staticMode(LedInfo * const led, const LightLocationInfo *const info)
    {
        candleMode(led, info);
        const int noise = random(-10, 40);
        led->brightness = globaliseBrightness(forceRange(led->extra + noise, 0, 100));
    }

    /***************************************************************************
     * @brief   Wave mode (clockwise)
     *          The peak of the circle is brightest, with the area surrounding
     *          it dimming gently to nothing at 180 degrees away.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void waveModeCw(LedInfo *const led, const LightLocationInfo * const info)
    {
        int angle = (int)(info->angle - led->angle);
        angle = abs(((angle + 180) % 360) - 180);
        led->brightness = globaliseBrightness(round((100.0f * (180 - angle)) / 180.0f));
    }

    /***************************************************************************
     * @brief   Wave mode (anti-clockwise)
     *          The peak of the circle is brightest, with the area surrounding
     *          it dimming gently to nothing at 180 degrees away.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void waveModeAcw(LedInfo *const led, const LightLocationInfo * const info)
    {
        int angle = (int)(info->angle - led->angle);
        angle = abs((((360 - angle) + 180) % 360) - 180);
        led->brightness = globaliseBrightness(round((100.0f * (180 - angle)) / 180.0f));
    }

    /***************************************************************************
     * @brief   Throb mode
     *          The lights all throb from off to on in unison.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void throbMode(LedInfo *const led, const LightLocationInfo * const info)
    {
        const float RADS_PER_DEGREE = 0.0174533f;
        const int value = 2 * round(abs(180.0f - info->angle));
        led->brightness = globaliseBrightness((1.0f + cos(value * RADS_PER_DEGREE)) * 50.0f);
     }

    /***************************************************************************
     * @brief   Throb mode
     *          The lights all throb from off to on in unison.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void throbMode2(LedInfo *const led, const LightLocationInfo * const info)
    {
        const float RADS_PER_DEGREE = 0.0174533f;
        const int value = 2 * round(abs(180.0f - info->angle));
        led->brightness = globaliseBrightness((1.0f + sin(value * RADS_PER_DEGREE)) * 50.0f);
    }

    /***************************************************************************
     * @brief   Heartbeat mode.
     *          Provides two pulses per revolution.
     *
     * @param   led     The LED to update
     * @param   info    The current light info, where the lead of the circle is
     */
    void heartbeatMode(LedInfo *const led, const LightLocationInfo * const info)
    {
        const int delta = round(min(abs(225.0f - info->angle), abs(135.0f - info->angle)));
        const int percent = round((100 * delta) / 135.0f);
        // const int value = round(((100 - percent) * 1.25f) - 25.0f);
        const int value = round(((100 - percent) * 2.0f) - 100.0f);

        led->brightness = globaliseBrightness(value);
    }



    /***************************************************************************
     * @brief   Adjusts the brightness of the input value by the global
     *          adjustment value/divider.
     *
     * @param   brightness  The brightness desired at full brightness
     *
     * @return  The brightness when scaled with the current brightness divider.
     */
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

    /***************************************************************************
     * @brief   Writes the current brightness levels to the LEDs within the
     *          cluster.
     */
    void updateLedBrightnesses()
    {
        for (int i = 0; i < count; ++i)
        {
            analogWrite(leds[i].pin, brightnessToDutyCycle(leds[i].brightness));
        }
    }

    /// @brief  Pointer to the LED information cluster.
    LedInfo *leds;

    /// @brief  The number of LEDs within this cluster.
    const int count;

    /// @brief  The current number of revolutions per minute (speed) of illuminations.
    float revsPerMinute;

    /// @brief  The time that this illumination pattern was started.
    long startTimeMs;

    /// @brief  The number of milliseconds per pattern revolution.
    long revTimePeriodMs;

    /// @brief  Accessor variable to read and write the settings to non-volatile memory.
    NonVol<Settings> settingsNV;

    /// @brief  The current settings.
    Settings settings;

    /// @brief  Whether the LED cluster is currently running/displaying patterns.
    bool running;
};
