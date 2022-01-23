# RoundMeter
CO2 measurement as a learning experiment with a round LCD. The device will show succesively a (dutch) weather forecast. the CO2 measurement in the room and a (dutch) forecast for the rain amount the next 2 hours.

## PARTS USED
- Waveshare 1.28 Round LCD
- Winsen MH-Z19B CO2 sensor 
- Wemos D1 mini lite ESP8266 micro controller
- USB micro cable connected to a power source

## Information sources
Thanks to the Dutch website Buienradar.nl which has the weather conditions in JSON format and a text based rain forecast for the next 2 hours. The CO2 information is read fromt he local MHZ sensor.

**Note:** The sensor is intended for the Dutch weather forecast. You can reuse parts and ideas for the rest of the world, change urls and interpretation of the output in the way you like.

## Build
### Connect CO2 Sensor
The Winsen CO2 sensor has a break-out board. From that board you need to connect four cables to the Wemos board:
- Green (RX) to Wemos D1
- Blue (TX) to Wemos D2
- Black (Gnd) to Wemos GND _(shared with display)_
- Red (5V) to Wemos 5V _(shared with display)_

### Connect Display
The WaveShare 1.18 Inch LCD display uses a SPI interface. Connect the following wires to the Wemos board:
- Blue (DC) to Wemos D3
- Yellow (CS) to Wemos D0
- Orange (CLK) to Wemos D5
- Green (DIN) to Wemos D7
- Purple (VCC) to Wemos 3V3
- White (GND) to Wemos GND _(shared with sensor)_
- Grey (BL) to Wemos 5V _(shared with sensor)_
- Brown (RST) to Wemos D4

### The Box
The box is designed with [TinkerCad](https://www.tinkercad.com/things/4qZ85xxE9la-128-round-lcd-2). Download or change the box there, or simply download the STL file from the /box design directory.

### Software
I used Visual Studio Code with the Platform IO extension to create and upload the code to the Wemos. It will work with e.g. the Arduino editor as well, but you might need to change some library references etc.

### Localization
Check the main.ccp file. On line 178 in the variable rain_link2 the latitude and longitude of the rain forecast location is mentioned. Change to a convineant lcation.
Also on line 179 the station id is mentioned for the Weather station. If you want to know alternatives, view the [JSON info](data.buienradar.nl/2.0/feed/json) online. Change to what fits you best.

Recompile the source and upload to the Wemos.

## INSTALLATION
This device is using the wifimanager library for an initial setup in a new environment. Power up the meter the first time in a new environment and give it some moments to start up. The device will create a Wifi network with the SSID "Kippenhok".

- Go to the settings on your phone
- Select the netwerk "Kippenhok"
- Your phone will connect to the network "Kippenhok"
- A webpage will pop-up automatic
- Select "Configure Wifi" 
- Select your own network in the list
- Add the password for this network
- Confirm

The Co2 meter device will now restart automatic and connect to the network you selected above. If this does not work for some reason, you need to repeat the Installation steps.

## OPERATIONAL

The meter will announce itself during boot phase. After that, the screen will refresh every 10 seconds witht he next screen.(see define SCREEN_CHANGE_TIMEOUT if you want to change that)
1. The Weather info screen will tell you some important weather parameters.
2. The CO2 info screen will tell you the current CO2 value. The outter ring will be green between 0 and 800, yellow between 800-1200 and red above 1200.
3. The rain info screen shows the amount of rain expected for the next 2 hours, in blocks of 10 minutes. The graph will be blue if the maximum expected rain is less than 5 mm/hour. If the maximum amount of rain is above 5 mm/hour and below 20 mm/hour than the graph will be in the magenta color. Above 20 mm/hour the graph will be shown in red.

If the meter is not working as intended connect an USB cable to a serial terminal. Use the Arduino terminal, or the built-in one in PlatformIO and read what is happening. Every step the Wemos makes is written to the console. Perhaps it gives you a clue what is happening.

