/**
 * @file    sketch_nuka_cola.ino
 *
 * @brief   Provides the top level control of the illuminated Nuka Cola stand.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @date    2020
 */
#include <string.h>
#include "Common.h"
#include "LedCluster.h"
#include "InputHelper.h"
#include "OutputHelper.h"

/**
 * Constants
 */

/// @brief  Serial input to increment the pattern.
#define PATTERN_MODE_CHAR       'P'
/// @brief  Serial input to increment the speed.
#define SPEED_MODE_CHAR         'S'
/// @brief  Serial input to increment the brightness.
#define BRIGHTNESS_MODE_CHAR    'B'
/// @brief  Serial input to set into running mode.
#define RUNNING_MODE_CHAR       'R'
/// @brief  Serial input to set into sleep mode.
#define SLEEP_MODE_CHAR         'X'
/// @brief  API request string.
#define API_REQUEST_STR         "api?"


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
 * @param   delta   The value to be added for the current setting mode
 *
 * @return  The value saved by the cluster for the current mode.
 */
static int toggleClusterValue(const int delta)
{
  int value = 0;
  if (cluster != nullptr)
  {
    switch (mode)
    {
      case SettingModes::Pattern:
        lastModeChange = millis();
        value = cluster->updatePattern(delta);
        modeLED = LOW;
        delay(80);
        modeLED = HIGH;
        break;

      case SettingModes::Brightness:
        lastModeChange = millis();
        value = cluster->updateBrightness(delta);
        brightnessLED = LOW;
        delay(80);
        brightnessLED = HIGH;
        break;

      case SettingModes::Speed:
        lastModeChange = millis();
        value = cluster->updateSpeed(delta);
        speedLED = LOW;
        delay(80);
        speedLED = HIGH;
        break;

      case SettingModes::Running: // Deliberate fall-through
      case SettingModes::Sleep:   // Deliberate fall-through
      default:
        // Nothing to do
        break;
    }
  }
  return value;
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
      mode = SettingModes::Running;
      cluster->startUp();
    }
    else if (SettingModes::Running == mode)
    {
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
    switch (mode)
    {
      case SettingModes::Pattern:
        setMode(SettingModes::Brightness);
        break;

      case SettingModes::Brightness:
        setMode(SettingModes::Speed);
        break;

      case SettingModes::Speed:
        setMode(SettingModes::Pattern);
        break;

      case SettingModes::Running:
        setMode(SettingModes::Pattern);

      case SettingModes::Sleep: // Deliberate fall-through
      default:
        break;
    }
  }
}

/*******************************************************************************
 * @brief   Sends the API to the connected serial device.
 */
static void sendApi()
{
  Serial.println("Letter in square brackets is the key.");
  Serial.println("Upper case increases, lower case decreases.");
  Serial.println("To assign value, use '=X' where X is the value to set.");
  Serial.println("Example: \"s=100\" sets Speed to 100%, and \"b\" decreases brightness one step.");
  Serial.print(String("Pattern [") + PATTERN_MODE_CHAR + "] ");
  for(int i = 0; i < Patterns::PATTERN_COUNT; i++)
  {
    Serial.print(String(PATTERN_STRINGS[i]) + " (" + i + "),");
  }
  Serial.println("");
  Serial.println(
    String("Speed [") + SPEED_MODE_CHAR +"] " +
    SpeedConstants::MIN_SPEED_PCT + "-" +
    SpeedConstants::MAX_SPEED_PCT
  );
  Serial.println(
    String("Brightness [") + BRIGHTNESS_MODE_CHAR + "] " +
    BrightnessConstants::MIN_BRIGHTNESS_PCT + "-" +
    BrightnessConstants::MAX_BRIGHTNESS_PCT
  );
  Serial.println(String("Running Mode [") + RUNNING_MODE_CHAR + "]");
  Serial.println(String("Sleep Mode [") + SLEEP_MODE_CHAR + "]");
}

/***************************************************************************
 * @brief   Gets the value assigned to the incoming command.
 *
 * @param   command  The command string
 * @param   chars    The number of characters in the command
 *
 * @return  The value if provided, otherwise zero.
 */
static int getIncomingValue(const char * const command, const size_t chars)
{
  const int offset = (command[0] == '=') ? 1 : 0;
  String value = "";
  for(int i = offset; i < chars; i++)
  {
    if (!isDigit(command[i]))
    {
      value = "0";
      break;
    }
    value += (char)command[i];
  }
  return value.toInt();
}

/***************************************************************************
 * @brief   Handles an incoming serial command.
 *
 * @param   command  The command to interpret
 * @param   chars    The number of characters in the command string
 */
static void handleSerialCommand(const char * const command, const size_t chars)
{
  const SettingModes currentMode = mode;
  if (cluster != nullptr && chars > 0)
  {
    const int compare = strncmp(command, API_REQUEST_STR, chars);
    if (strncmp(command, API_REQUEST_STR, strlen(API_REQUEST_STR)) == 0)
    {
      sendApi();
    }
    else
    {
      const char cmd = toupper(command[0]);
      const bool inc = cmd == command[0];
      const bool testValue = chars > 1;
      int newValue = 0;
      // For pattern, speed and brightness, if it looks like there's a value
      // provided, test for it, otherwise increment or decrement based on the
      // case of the command.
      switch (cmd)
      {
        case PATTERN_MODE_CHAR:
          {
            int pattern = 0;
            if (testValue)
            {
              newValue = getIncomingValue(command + 1, chars - 1);
              pattern = cluster->setPattern(newValue);
            }
            else
            {
              setMode(SettingModes::Pattern);
              pattern = toggleClusterValue(inc ? 1 : -1);
              setMode(currentMode);
            }
            Serial.println(String(PATTERN_MODE_CHAR) + "=" + pattern);
          }
          break;

        case SPEED_MODE_CHAR:
          {
            int speed = 0;
            if (testValue)
            {
              newValue = getIncomingValue(command + 1, chars - 1);
              speed = cluster->setSpeedPercent(newValue);
            }
            else
            {
              setMode(SettingModes::Speed);
              speed = LedCluster::toSpeedPercentage(toggleClusterValue(inc ? 1 : -1));
              setMode(currentMode);
            }
            Serial.println(String(SPEED_MODE_CHAR) + "=" + speed);
          }
          break;

        case BRIGHTNESS_MODE_CHAR:
          {
            int brightness = 0;
            if (testValue)
            {
              newValue = getIncomingValue(command + 1, chars - 1);
              brightness = cluster->setBrightnessPercent(newValue);
            }
            else
            {
              setMode(SettingModes::Brightness);
              brightness = LedCluster::toBrightnessPercentage(toggleClusterValue(inc ? 1 : -1));
              setMode(currentMode);
            }
            Serial.println(String(BRIGHTNESS_MODE_CHAR) + "=" + brightness);
          }
          break;

        case RUNNING_MODE_CHAR:
          setMode(SettingModes::Running);
          cluster->startUp();
          Serial.println(RUNNING_MODE_CHAR);
          break;

        case SLEEP_MODE_CHAR:
          setMode(SettingModes::Sleep);
          cluster->shutdown();
          Serial.println(SLEEP_MODE_CHAR);
          break;

        default:
          Serial.println(String("Unknown command: ") + command);
          sendApi();
          break;
      }
    }
  }
}

/*******************************************************************************
 * @brief   Check for serial connection and any incoming requests.
 */
static void pollSerial()
{
  if (Serial)
  {
    const size_t available = Serial.available();
    if (available != 0)
    {
      char *command = new char[available + 1];
      Serial.readBytes(command, available);

      // Strip new line
      if (command[available - 1] == '\n')
      {
        command[available - 1] = '\0';
      }
      else
      {
        command[available] = '\0';
      }
      handleSerialCommand(command, strlen(command));
      delete[] command;
      command = nullptr;
    }
  }
}

/*******************************************************************************
 * @brief   Sets up the required global variables and communications.
 */
void setup()
{
  Serial.begin(9600);
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

  pollSerial();
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
