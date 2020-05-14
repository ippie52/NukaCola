# NukaCola

## What is this for?
This is an Arduino Nano project aimed specifically for a more exciting display of a Fallout themed decoration. The code here is customised for my display, however could easily be adapted for other displays.

I have deliberately split the code into fairly logical files, however I have not bothered extracting the code into headers and source CPP files as my aim was to get this working, not present this to a customer with all the bells and whistles.

That being said, my occupation requires I be thorough with documentation and so it heavily commented, which will hopefully help anyone wishing to adapt this code for their own projects.

There are several 'features' that I would like to fix, but currently they don't cause any breakages, so they may never be changed.

## Hardware

### Bill of materials (electronics)
This is aimed at the Arduino Nano boards, and therefore the pin mappings may not be correct if you are using another platform.
As well as an Arduino Nano, I also used:
- Arduino Nano
- 6 x 5mm UV LEDs (For illuminating the bottle)
- 6 x 220 Ohm resistors (for the UV LEDs)
- 3 x Push to make switches (for changing the illumination)
- 3 x 10k Ohm resistors (pull-downs for the inputs)
- 3 x 3mm LEDs (for showing what settings are being updated)
- 3 x 330 Ohm resistors (for the display LEDs)
- Strip board

### Stand
I have placed the STL for the stand on [thingiverse](https://www.thingiverse.com/thing:4352918). I printed this using blue glow in the dark PLA, which works rather nicely. This was printed with the bottle in mind, where the 5mm UV LEDs are mounted to fit just under the base of the bottle, are evenly spaced at 60° from one another in a circle, and angled towards the water line within the bottle (around 7° off the vertical). Once printed, I also painted the logo on the front with white Acrylic, which blocks the light when showing to keep the logo visible.

For more info on the stand, please visit the link above.

### Bottle
The bottle used was an old 330ml Coca Cola bottle with a Nuka Cola Quantum label and bottle top printed and stuck on. The contents is tonic water (quinine fluoresces under UV light), with some blue curaçao for the blue tinge.

### Pin map
| Arduino Nano Pin | Connected To | Notes |
| ----- | ----------------- | ----------------------------------------------------------------- |
| 3     | Display LED 1     |  Display output (5mm UV LED with 220 Ohm resistor)                |
| 5     | Display LED 2     |  Display output (5mm UV LED with 220 Ohm resistor)                |
| 6     | Display LED 3     |  Display output (5mm UV LED with 220 Ohm resistor)                |
| 9     | Display LED 4     |  Display output (5mm UV LED with 220 Ohm resistor)                |
| 10    | Display LED 5     |  Display output (5mm UV LED with 220 Ohm resistor)                |
| 11    | Display LED 6     |  Display output (5mm UV LED with 220 Ohm resistor)                |
| 11    | Display LED 6     |  Display output (5mm UV LED with 220 Ohm resistor)                |
| 2     | Mode LED          | Indicator output for 'mode' (3mm LED with 330 Ohm resistor)       |
| 4     | Speed LED         | Indicator output for 'speed' (3mm LED with 330 Ohm resistor)      |
| 7     | Brightness LED    | Indicator output for 'brightness' (3mm LED with 330 Ohm resistor) |
| 8     | Setting Input     | Input for changing the current setting mode                       |
| 12    | Up Input          | Input for incrementing the current setting value                  |
| 13    | Down Input        | Input for decrementing the current setting value                  |

## Patterns and speeds
There are several [illumination patterns](https://imgur.com/gallery/nqhQovs) that provide interesting effects. To obtain these effects, the LEDs are treated as being in a circle, with the first LED in the cluster being at 0°, then each other being evenly spaced (depending on the number of LEDs). With six LEDs, they are therefore at 60° from one another, i.e. 0°, 60°, 120°, 180°, 240° and 300°.

With the concept of a circle, the speed of the illumination pattern is effectively the number of revolutions around the circle per minute, ranging from 6 (once every ten seconds) to 60 (once per second). Different patterns appear better at different speeds, so play around until you find a nice combination.

Considering the speed, the time since the LED cluster was started can therefore determine the position around the circle and the current revolution. I call this the head or lead angle.

![Image of wave pattern](https://github.com/ippie52/NukaCola/blob/master/images/wave.gif?raw=true)

### Just On
This is simple. The LEDs are just on at the brightness level set.

### Chase (Clockwise, Anticlockwise and Both)
For this pattern, the lead angle will be at the maximum brightness whilst one degree behind it will be off. This means the LED closest behind the lead angle will illuminate from off to full then dim slowly. This pattern is available clockwise, (LEDs 1 to 6) or anticlockwise (6 to 1), and both, where the lead angle is mirrored around 0° and 180°, giving two lead angles.

### Wave (Clockwise and Anticlockwise)
The brightness around the lead angle ramps up and down, with 180° being fully off. This causes the brightness to move around the circle in a wave, both clockwise and anticlockwise.

### Throb
The throb is a simple cosine wave of the current lead angle, with all LEDs being at the same brightness.

### Throb Two
This is similar to the previous, however provides a double pulse. Using a sine wave that is reflected around 180° there is a ramp up, back to half brightness, the back up to full. The pulse then drops to zero, back up to half brightness, then down to zero again before repeating the pattern. I found this by accident when messing up the equation, but I liked it so left it in.

### Heartbeat
This pattern is a rough attempt to look like a heartbeat.

### Raindrop
Each LED will flash at a random time once per revolution. The flash will be a sudden ramp up with slower drop down.

### Flames
This is supposed to look like flames flickering. It's not perfect and for some reason it always ends up on the low end of brightness, but it seems good enough for the purpose!

### Static/Noise
This is based on the flames pattern, but introduces additional spikes of noise which give a much more random pattern.

## Controls
### Manual controls
If using the buttons and LEDs attached to the board, the controls are fairly self explanatory:

#### Setting Button
When pressed, this should trigger the pattern mode LED to light up. Subsequent presses will then cause the brightness then speed LEDs to light up. When these mode LED indicators are lit, they indicate that the particular setting can be updated by pressing the up or down buttons. If no buttons are pressed for over 10 seconds, they LEDs will turn off and running or sleep more will continue.

Pressing the setting button for more than 2 seconds will toggle the mode from running to sleep mode. Pressing again for a further 2 seconds will put it back into running mode.

#### Up Button
When in pattern mode, this will select the next illumination pattern. This will always trigger a change as the patterns will loop. When in brightness mode, it will increase the brightness until it reaches 100%. When in speed mode, it will increase the speed until it reaches 100%.

#### Down Button
When in pattern mode, this will select the previous illumination pattern. This will always trigger a change as the patterns will loop. When in brightness mode, it will decrease the brightness until it reaches 5% (the lowest value permitted). When in speed mode, it will decrease the speed until it reaches 10% (the lowest speed permitted).

### Serial Comms
The API for this is fairly basic, allowing for simple strings to be used to set and alter values.

#### API Query
By sending the string "api?" (or any unrecognised command), a rough guide to the API will be sent via the serial connection.

#### Pattern
The letter 'P' is used to update the illumination pattern. Sending an upper case 'P' will increment the pattern, whilst a lower case will decrement it. To set a particular pattern index, use 'p=X', where X is the pattern index (the case of the 'p' is not important here)

#### Brightness
The letter 'B' is used to update the brightness value. Like with the pattern controls, an upper case 'B' will increase the brightness, whilst a lower case 'b' will decrease the brightness. The brightness level cannot be increased or decreased below the minimum and maximum values. To set the value exactly, use 'b=XXX', where XXX is the brightness percentage.

#### Speed
The letter 'S' is used to update the speed value. Like with the pattern and brightness controls, an upper case 'S' will increase the speed, whilst a lower case 'b' will decrease the speed. The speed level cannot be increased or decreased below the minimum and maximum values. To set the value exactly, use 's=XXX', where XXX is the speed percentage.

#### Sleep mode
To turn off the LED cluster, use the command 'X' (case insensitive).

#### Running mode
To bring the LED cluster out of sleep mode, use the command 'R' (case insensitive).

## How all this came to be
The Hallowe'en before last, I went with a Fallout themed costume. Along with that, I made a glowing Nuka Cola Quantum bottle and a little stand made from some strip board, a 9 volt battery, some resistors and UV LEDs.

Since then, it's been sat above my desk gathering dust.

I recently had reason to start looking at Arduino boards, and whilst doing so, looked up and thought about my Nuka Cola Quantum bottle. I also thought about how it would be nice to have a proper stand for it, and then decided to make it a reality.

I created a stand from blue glow in the dark PLA on my 3D printer and put together an Arduino Nano board to control the UV LEDs for a more exciting display.

Provides software and hardware for creating a fancy Nuka Cola bottle stand
