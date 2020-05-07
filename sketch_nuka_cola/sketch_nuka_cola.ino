/**
 * @file    sketch_nuka_cola.ino
 *
 * @brief   Provides the top level control of the illuminated Nuka Cola stand.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#include "Common.h"
#include "LedCluster.h"
#include "InputHelper.h"
#include "OutputHelper.h"

/**
 * Forward declarations
 */
static void upBtnToggled(const int, const int state, const long);
static void downBtnToggled(const int, const int state, const long);
static void powerTimeout(const int, const long);
static void settingBtnToggled(const int, const int state, const long durationMs);

/// @brief  The available modes - used to identify what input signals do.
enum SettingModes
{
  Sleep,
  Running,
  Pattern,
  Brightness,
  Speed
};

/// @brief  Pointer to an LED cluster, used to create illumination patterns.
LedCluster *cluster = nullptr;

/// @brief  Up button input.
InputHelper upBtn(Pins::SettingUpBtn, upBtnToggled);
/// @brief  Down button input.
InputHelper downBtn(Pins::SettingDownBtn, downBtnToggled);
/// @brief  Settings/Mode selection input.
InputHelper settingSelectionBtn(
  Pins::SettingSelectionBtn,
  settingBtnToggled,
  powerTimeout,
  2000
);

/// @brief  The mode LED indicator.
OutputHelper modeLED(Pins::ModeLED);
/// @brief  The speed LED indicator.
OutputHelper speedLED(Pins::SpeedLED);
/// @brief  The brightness LED indicator.
OutputHelper brightnessLED(Pins::BrightnessLED);

/// @brief  The current mode.
SettingModes mode = SettingModes::Running;
/// @brief  Stores when the last mode change occurred.
long lastModeChange = 0;

/*******************************************************************************
 * @brief   Toggles the cluster value for the given setting mode.
 *
 * @param   The value to be added for the current setting mode
 */
static void toggleClusterValue(const int value)
{
  if (cluster != nullptr)
  {
    switch (mode)
    {
      case SettingModes::Pattern:
        lastModeChange = millis();
        Serial.println("Updating pattern...");
        if (cluster->updatePattern(value))
        {
          modeLED = LOW;
          delay(80);
          modeLED = HIGH;
        }
        break;

      case SettingModes::Brightness:
        lastModeChange = millis();
        Serial.println("Updating brightness...");
        if (cluster->updateBrightness(value))
        {
          brightnessLED = LOW;
          delay(80);
          brightnessLED = HIGH;
        }
        break;

      case SettingModes::Speed:
        lastModeChange = millis();
        Serial.println("Updating speed...");
        if (cluster->updateSpeed(value))
        {
          speedLED = LOW;
          delay(80);
          speedLED = HIGH;
        }
        break;

      case SettingModes::Running: // Deliberate fall-through
      case SettingModes::Sleep:   // Deliberate fall-through
      default:
        // Nothing to do
        break;
    }
  }
}

/***************************************************************************
 * @brief   Handles the up button presses.
 *
 * @param   state  The current state of the button press
 */
static void upBtnToggled(const int, const int state, const long)
{
  if (state && cluster != nullptr)
  {
    Serial.println("Up button pressed");
    toggleClusterValue(1);
  }
}

/***************************************************************************
 * @brief   Handles the down button presses.
 *
 * @param   state  The current state of the button press
 */
static void downBtnToggled(const int, const int state, const long)
{
  if (state && cluster != nullptr)
  {
    Serial.println("Down button pressed");
    toggleClusterValue(-1);
  }
}

/***************************************************************************
 * @brief   Time-out function called when the settings/mode button has been
 *          pressed for a prolonged period of time. This will either put the
 *          LED cluster to sleep, or wake it up.
 */
static void powerTimeout(const int, const long)
{
  if (cluster != nullptr)
  {
    if (SettingModes::Sleep == mode)
    {
      Serial.println("Powering up");
      mode = SettingModes::Running;
      cluster->startUp();
    }
    else if (SettingModes::Running == mode)
    {
      Serial.println("Powering down");
      mode = SettingModes::Sleep;
      cluster->shutdown();
    }
  }
}

/***************************************************************************
 * @brief   Sets the current mode for input signals.
 *
 * @param   newMode  The new mode to change to
 */
static void setMode(const int newMode)
{
  mode = newMode;
  modeLED = SettingModes::Pattern == mode;
  brightnessLED = SettingModes::Brightness == mode;
  speedLED = SettingModes::Speed == mode;
  lastModeChange = millis();
}

/***************************************************************************
 * @brief   Handler for when the setting button is pressed.
 *
 * @param   state      The current state of the button
 */
static void settingBtnToggled(const int, const int state, const long)
{
  if (state && cluster != nullptr)
  {
    Serial.println("Mode button pressed");
    switch (mode)
    {
      case SettingModes::Pattern:
        setMode(SettingModes::Brightness);
        Serial.println("Now in Brightness mode");
        break;

      case SettingModes::Brightness:
        setMode(SettingModes::Speed);
        Serial.println("Now in Speed mode");
        break;

      case SettingModes::Speed:
        setMode(SettingModes::Pattern);
        Serial.println("Now in Pattern mode");
        break;

      case SettingModes::Running:
        setMode(SettingModes::Pattern);
        Serial.println("Now in Pattern mode");

      case SettingModes::Sleep: // Deliberate fall-through
      default:
        break;
    }
  }
}

/*******************************************************************************
 * @brief   Sets up the required global variables and communications.
 */
void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    delay(10);
  }
  Serial.println("Starting!");

  byte ledPins[] = {
    Pins::DisplayLED1,
    Pins::DisplayLED2,
    Pins::DisplayLED3,
    Pins::DisplayLED4,
    Pins::DisplayLED5,
    Pins::DisplayLED6
  };

  cluster = new LedCluster(ledPins, 6);

  // Seed the randomiser with the current noise on analogue input zero
  randomSeed(analogRead(0));
}

/*******************************************************************************
 * @brief   Loop function, runs continually.
 */
void loop()
{

  // Check the inputs for any changes
  upBtn.poll();
  downBtn.poll();
  settingSelectionBtn.poll();

  // Update the LED cluster levels
  cluster->poll();

  // If the speed, brightness or pattern is currently being changed, time out
  // after a certain time, back into running mode and turn off the setting LEDs
  if (mode != SettingModes::Running && mode != SettingModes::Sleep)
  {
    const long delta = millis() - lastModeChange;
    if (delta > 10000)
    {
      setMode(SettingModes::Running);
      lastModeChange = 0;
    }
  }
}
