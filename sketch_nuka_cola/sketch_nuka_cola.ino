/**
 * @file    sketch_nuka_cola.ino
 *
 * @brief   Provides the different LED modes for the Nuka Cola stand.
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

byte ledPins[] = {
  Pins::DisplayLED1,
  Pins::DisplayLED2,
  Pins::DisplayLED3,
  Pins::DisplayLED4,
  Pins::DisplayLED5,
  Pins::DisplayLED6
};

enum SettingModes
{
  Sleep,
  Running,
  Pattern,
  Brightness,
  Speed
};


LedCluster *cluster = nullptr;
InputHelper upBtn(Pins::SettingUpBtn, upBtnToggled);
InputHelper downBtn(Pins::SettingDownBtn, downBtnToggled);
InputHelper settingSelectionBtn(
  Pins::SettingSelectionBtn,
  settingBtnToggled,
  powerTimeout,
  2000
);

OutputHelper modeLED(Pins::ModeLED);
OutputHelper speedLED(Pins::SpeedLED);
OutputHelper brightnessLED(Pins::BrightnessLED);

SettingModes mode = SettingModes::Running;
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

static void upBtnToggled(const int, const int state, const long)
{
  if (state && cluster != nullptr)
  {
    Serial.println("Up button pressed");
    toggleClusterValue(1);
  }
}

static void downBtnToggled(const int, const int state, const long)
{
  if (state && cluster != nullptr)
  {
    Serial.println("Down button pressed");
    toggleClusterValue(-1);
  }
}

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

static void setMode(const int newMode)
{
  mode = newMode;
  modeLED = SettingModes::Pattern == mode;
  brightnessLED = SettingModes::Brightness == mode;
  speedLED = SettingModes::Speed == mode;
  lastModeChange = millis();
}


static void settingBtnToggled(const int, const int state, const long durationMs)
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


void setup() {
  Serial.begin(9600);
  while (!Serial)
  {
    delay(10);
  }
  // put your setup code here, to run once:
  Serial.println("Starting!");
  cluster = new LedCluster(ledPins, 6);

  // if analog input pin 0 is unconnected, random analog
  // noise will cause the call to randomSeed() to generate
  // different seed numbers each time the sketch runs.
  // randomSeed() will then shuffle the random function.
  randomSeed(analogRead(0));
}

void loop() {
  cluster->poll();
  upBtn.poll();
  downBtn.poll();
  settingSelectionBtn.poll();

  if (mode != SettingModes::Running && mode != SettingModes::Sleep)
  {
    const long delta = millis() - lastModeChange;
    if (delta > 20000)
    {
      setMode(SettingModes::Running);
      lastModeChange = 0;
    }
  }
  delay(20);
}
