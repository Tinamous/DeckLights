DeckLights - Arduino Ethernet powered NeoPixel deck lights.
-----------------------------------------------------------

Connects to Tinamous.com via MQTT to publish status (e.g. light change) and to receive requests for colour changes.

Libraries:
----------
* MQTT - https://github.com/Tinamous/pubsubclient - http://knolleary.net/arduino-client-for-mqtt/
* Adafruit Neopixels - https://github.com/adafruit/Adafruit_NeoPixel

Both libraries were placed in the Arduino program libraries folder (NeoPixel library renamed as per instructions).

Hardware:
---------
* Arduino Ethernet (with or without POE module. POE module can power about 26-ish pixels on full white).
* Adafruit NeoPixels - adafru.it/1312 used for individual pixels. Other modules work as well (e.g. 12 pixel ring - adafru.it/1463)


Setup:
------
Change Server, userName and password to be your own values.

If you are using Tinamous for the MQTT Broker:
port = 1883;
server[] = "[AccountName].Tinamous.com"; //
userName[] = "[DeviceUserName].[AccountName]"; // (e.g. KitchenLights.demo for the demo account).
password[] = "[the password]";

You can make a free account at https://tinamous.com/Registration

If you use another MQTT broker then assign as appropriate.

In InitializeMQTT set the client name, and modify setup publishing and subscription.

Currently set to:
* Publish to the Tinamous status timeline.
* Subscribe to the user @KitchenLights feed.

Post a message to @KitchenLight with a #rrggbb code will alter the lights colour.

Wiring:
-------

NeoPixels are connected to +5V and Gnd with a capacitor, and pin 7 with a low value resitor as per Adafruit instructions.

A switch is connected on pin 2 (push to make, connect to ground) and LED light for the switch is connected to pin 5.

Work in progress: A PIR sensor connector is on pin 3 and powered from pin 6 (so it can be disabled / power reduced).

Pixel count is set to 14, however only 4 are used, this allows a 10 pixel strip to be added without reprogramming in the future.

Startup:
--------
* On startup the first 4 pixels are set to blue.
* If the Ethernet DHCP fails the first (0) pixel is set to red, otherwise it goes green.
* If connection to MQTT broker fails the second (1) pixel is set to red, otherwise it goes green.

The device is left in that state (2 green/2 blue) after startup. Pressing the button will switch the lights off, then again to on, where the lights will go to full blue. They may then be set from MQTT messages.
