/* *****************************************************************************

   Copyright (C) 2022 P.A.G.M. Zengers

   This program shows weather and environmental information a a round 
   LCD screen. The weather information is intended for the Netherlands. 
   You can select the info via a potmeter hoe heet zon ding

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

   You can find my contact details at <https://github.com/idurz/>

   Environment       : PlatformIO
   Settings          : platform = espressif8266
                     : board = d1_mini_lite
                     : framework = arduino
                      +-----------------------------+
                      |                             |
                      |     +--------------------+  |
   +---------+        |     |    WEMOS D1 MINI   |  |
   |WAVESHARE|        |     |                    |  |       +---------+
   |         |        |     ()rst RST         TX()  |       | MHZ-19B |
   |     DC ()-blue---+     ()A0 ADC0         RX()  |       |         |
   |     CS ()-yellow-------()D0 WAKE     SCL D1()--|green--()RX      |
   |    CLK ()-orange-------()D5 SCLK     SDA D2()--|blue---()TX      |
   |         |              ()D6 MISO   FLASH D3()--+       |         |
   |    DIN ()-green--------()D7 MOSI         D4()--------+ |         |
   |         |              ()D8 CS          GND()-+-black|-()GND     |
   |    VCC ()-purple-------()3V3             5V()-|-red+-|-()5V      |
   |    GND ()-white-----+  +--------------------+ |    | | +---------+
   |     BL ()-grey----+ |                         |    | |
   |    RST ()-brown-+ | +-------------------------+    | | 
   +---------+       | +--------------------------------+ |  
                     +------------------------------------+
 * ****************************************************************************/


/* *****************************************************************************
   Include the necessary libraries here
 * *****************************************************************************/

#include <Arduino.h>
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <DNSServer.h>            // needed for library above
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <TFT_eSPI.h>             // Include the graphics library (incl sprite)
#include <SoftwareSerial.h>
#include <MHZ19.h>
#include "WeatherSymbols.h"       // Our pictures in a C array

/* *****************************************************************************
   Defines, to ensure our code is easier to maintain
 * ****************************************************************************/

#define VERSION                   "1.0.0"

// ip based
#define DEVICE_NAME               "Kippenhok" // Name in station mode

// Pin mapping
#define PIN_D0                     16
#define PIN_D1                     5
#define PIN_D2                     4
#define PIN_D3                     0
#define PIN_D4                     2
#define PIN_D5                     14
#define PIN_D6                     12
#define PIN_D7                     13
#define PIN_D8                     15
#define PIN_D9                     3
#define PIN_D10                    1

// Serial port config
#define DEBUG_OUTPUT_BAUDRATE      9600  // Hardware serial (USB)
#define SENSOR_OUTPUT_BAUDRATE     9600  // Software serial

// Screen definitions
#define SCREEN_CHANGE_TIMEOUT      10000 // ms to wait before showing another screen
#define SCREEN_WEATHER             1     // ID for Show_Weather info
#define SCREEN_CO2                 2     // ID for Show_CO2 info
#define SCREEN_RAIN                3     // ID for Show_CO2 info
#define SCREEN_COUNT               3     //  screens available

#define RESET_DELAY_ON_ERROR       15000 // (ms) delay after panic to ensure last message is read
#define ESTABLISH_DELAY            500   // (ms) delay after important tasks to settle down

#define JSON_INTERVAL_SEC          600   // Interval time (sec) between for retrieving weather info
#define RAIN_INTERVAL_SEC          600   // Interval time (sec) between for retrieving buienradar info
#define CO2_INTERVAL_SEC           5     // Interval time (sec) between for retrieving CO2 info

#define HTTPS_TIMEOUT_SEC          15    // (sec) timeout for https call

#define TEXT_SIZE_SMALL            1     // 16 pixels high
#define TEXT_SIZE_MEDIUM           2     // 26 pixels high
#define TEXT_SIZE_LARGE            4     // 48 pixels high
#define TEXT_SIZE_XLARGE           6
#define TEXT_MAX_LENGTH            30

#define SYMBOL_WIDTH               50
#define SYMBOL_HEIGTH              50

// Meter colour schemes
#define RED2RED                    0
#define GREEN2GREEN                1
#define BLUE2BLUE                  2
#define BLUE2RED                   3
#define GREEN2RED                  4
#define RED2GREEN                  5
#define THREECOLOR                 6

// Set the the position and inner radius of the CO2 meter
#define METER_XPOS                 0
#define METER_YPOS                 0
#define METER_RADIUS               120
#define METER_MINVALUE             0
#define METER_MAXVALUE             2400

// Position info for the rain graph
#define RAIN_TOPX                  40
#define RAIN_TOPY                  40
#define RAIN_XLEN                  161
#define RAIN_YLEN                  160
#define RAIN_READINGS              24

/* MHZ Error codes  0   NULL, Library logic error, should not occur
                    1   OK
                    2   Timeout waiting for response
                    3   Syntax error in received data
                    4   CRC error in received data
                    5   Filter kicked in (value within 30 seconds of reset)
                    6   Failed   */

/* *****************************************************************************
   Global variables out of scope of functions
 * *****************************************************************************/
int                  MHZ_CO2,            // CO2 value of MHZ
                     MHZ_Temp;           // Indoor temp from MHZ

String               MHZ_Error,          // Last error message from MHZ
      	             WTH_timestamp,      // "2022-01-02T12:30:00",
		                 WTH_description,    // "Zwaar bewolkt",
                     WTH_icon,           // "https://www.buienradar.nl/resources/images/weather/30x30/c.png"
                     WTH_winddirection,  // "ZW",
                     WTH_sunrise,
                     WTH_sunset,
                     WTH_error,
                     WTH_airpressure,    // 1010.4,
		                 WTH_temperature,    // 12.3,
	                   WTH_windspeed,      // 5.7,
		                 WTH_humidity,       // 83.0,
			               WTH_precipitation,  // 0.0,
			               WTH_sunpower,       // 61.0,
			               WTH_rainFallLast24Hour, // 3.9,
			               WTH_rainFallLastHour; // 0.0,

String               RAIN_error;
float                Rain[RAIN_READINGS];         // Rain forecast in mm/hour
float                Rain_max;

// Weather data
const char*          json_host = "data.buienradar.nl";
const char*          json_link = "/2.0/feed/json";
unsigned long        json_next_get = 0;
const char*          rain_host = "gpsgadget.buienradar.nl";
const char*          rain_link1 = "/data/raintext?";
const char*          rain_link2 = "lat=52.14&lon=5.58"; 
String               stationid = "\"stationid\":6260";
unsigned long        rain_next_get = 0;
int                  screen_to_show =0; 

WiFiClientSecure httpsClient; // https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure
SoftwareSerial   sensor(PIN_D1, PIN_D2); //rx, tx
MHZ19            mhz(&sensor); 
TFT_eSPI         tft = TFT_eSPI();  // Create object "tft"

unsigned int rainbow(byte value) {
/* *************************************************************************************************
   rainbow

   Return a 16 bit rainbow colour
 * *************************************************************************************************/

  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = red through to 127 = blue

  byte red   = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue  = 0; // Blue is the bottom 5 bits

  byte sector = value >> 5;
  byte amplit = value & 0x1F;

  switch (sector)
  {
    case 0:
      red   = 0x1F;
      green = amplit;
      blue  = 0;
      break;
    case 1:
      red   = 0x1F - amplit;
      green = 0x1F;
      blue  = 0;
      break;
    case 2:
      red   = 0;
      green = 0x1F;
      blue  = amplit;
      break;
    case 3:
      red   = 0;
      green = 0x1F - amplit;
      blue  = 0x1F;
      break;
  }
  return red << 11 | green << 6 | blue;

}

void ringMeter(int value, int vmin, int vmax, int x, int y, int r, String units, int scheme) {
/* *************************************************************************************************
   ringMeter

   Draw the meter on the screen, returns x coord of righthand side
 * *************************************************************************************************/
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  
  x += r; y += r;   // Calculate coords of centre of ring

  // int w = r / 4;    // Width of outer ring is 1/4 of radius
  int w = r / 6;    // Width of outer ring is 1/6 of radius
  
  int angle = 150;  // Half the sweep angle of meter (300 degrees)

  int text_colour = 0; // To hold the text colour

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 5; // Segments are 5 degrees wide = 60 segments for 300 degrees
  byte inc = 5; // Draw segments every 5 degrees, increase to 10 for segmented ring

  // Draw colour blocks every inc degrees
  for (int i = -angle; i < angle; i += inc) {

    // Choose colour from scheme
    int colour = 0;
    switch (scheme) {
      case 0: colour = TFT_RED; break; // Fixed colour
      case 1: colour = TFT_GREEN; break; // Fixed colour
      case 2: colour = TFT_BLUE; break; // Fixed colour
      case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
      case 4: colour = rainbow(map(i, -angle, angle, 63, 127)); break; // Green to red (high temperature etc)
      case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
      case 6: if (i < -50) colour = TFT_GREEN; 
              else if (i < 0) colour = TFT_YELLOW;
                    else colour = TFT_RED;
              break;
      default: colour = TFT_BLUE; break; // Fixed colour
    }

    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_LIGHTGREY);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_LIGHTGREY);
    }
  }

  // Set the text colour
  tft.setTextColor(text_colour, TFT_BLACK);
  
  // Print value, if the meter is large then use big font 6, othewise use 4
  if (r > 84) tft.drawCentreString(" " + String(value) + " ", x - 5, y - 20, TEXT_SIZE_XLARGE); // Value in middle
  else tft.drawCentreString(" " + String(value) + " ", x - 5, y - 20, TEXT_SIZE_LARGE); // Value in middle

  // Print units, if the meter is large then use big font 4, othewise use 2
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (r > 84) tft.drawCentreString(units, x, y + 30, TEXT_SIZE_LARGE); // Units display
  else tft.drawCentreString(units, x, y + 5, TEXT_SIZE_MEDIUM); // Units display
}

void configModeCallback (WiFiManager *myWiFiManager) {
/* *************************************************************************************************
   Wifi Connect Manager

   From the WifiManager library. Will be called when in station mode. Gives it
   the opportunity to inform the user or do other things
 * *************************************************************************************************/
  Serial.println(F("Station mode; My ip:"));
  Serial.println(WiFi.softAPIP());
  Serial.println("SSID: " + String(myWiFiManager->getConfigPortalSSID()));

  //tft.setCursor( 0, 12); tft.print("Station mode; My ip:");
  //tft.setCursor( 0, 24); tft.print(WiFi.softAPIP());
  //tft.setCursor( 0, 36); tft.print(MSG_SSID);
  //tft.print(myWiFiManager->getConfigPortalSSID());

}

float mmHour(int radarvalue) {
/* *************************************************************************************************
   mmHour

   Calculate the rain fall in mm/hour by using the values from buienradar and their logaritmic type
   of scale like this 10^((value -109)/32) (example: 77 = 0.1 mm/hour)
 * *************************************************************************************************/
  return (pow(10, (radarvalue - 109) / 32.0) * 10.0) / 10.0;
}

void Show_Rain() {
  /* *************************************************************************************************
   Show_Rain

   Gets the global saved rain info fetched from the URL and displays on the TFT screen
 * *************************************************************************************************/

  Serial.println(F("Executing Show_Rain"));
  tft.startWrite();
  tft.fillScreen(TFT_BLACK); // Clear the screen first

  // Top level info

  tft.setTextColor(TFT_SKYBLUE);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.drawCentreString(String(WTH_rainFallLast24Hour) + " mm",METER_RADIUS,15,TEXT_SIZE_SMALL);
  tft.drawCentreString("Laatste 24 uur",METER_RADIUS,27,TEXT_SIZE_SMALL);
  
  // Bottom text
  tft.setTextColor(TFT_SKYBLUE);
  tft.drawCentreString("Regen komende 2 uur",METER_RADIUS,210,TEXT_SIZE_SMALL); // Value in middle
  tft.drawCentreString("BuienRadar.nl",METER_RADIUS,220,TEXT_SIZE_SMALL); // Value in middle


  //Rain_max = 4; // for testing


  // If there is no rain expected, don't bother printing, but give a simple message and return
  if (Rain_max < 0.01) {
    tft.setCursor(10, 118);
    tft.setTextSize(TEXT_SIZE_MEDIUM);
    tft.print("Geen regen voorzien");
    tft.setCursor(10, 140);
    tft.print("  komende twee uur");
   
    tft.drawLine(105,230,135,230,TFT_MAROON);
    for (int j=0;j<(SCREEN_CHANGE_TIMEOUT / 1000);j++) {
      delay(1000);
      tft.drawPixel(105+(j*3),230,TFT_SKYBLUE);
      tft.drawPixel(105+1+(j*3),230,TFT_SKYBLUE);
    }
    Serial.println(F("Completed Show_Rain"));
    return;
  }

  // Arriving here, we not that it will rain coming 2 hours
  // Make fixed definitions of 5 and 20 mm or higher if it is really wet
  int bar_color = TFT_RED;
  if (Rain_max <= 5) {
    Rain_max = 5;
    bar_color = TFT_BLUE;
  } else {
    if (Rain_max <= 20) {
      Rain_max  = 20;
      bar_color = TFT_MAGENTA;
    } else {
      Rain_max = 100;
    }
  }

  // Calculate the value for the two reference lines
  float lowline = round(Rain_max * 10 / 3) / 10;  // at 1/3 of graph
  float highline = round(Rain_max * 20 / 3) / 10; // at 2/3 of graph

  // Draw graph border
  tft.drawRect(RAIN_TOPX-1,RAIN_TOPY-1,RAIN_XLEN+1,RAIN_YLEN+2,TFT_NAVY);

  // We have 0..23 values; every 7 dots we need to print a new one. 
  // Also we need to fill the intermediate ones. total using 161 dots
  int x_axes = 40;
  int y_len, y_len_next;
  for (int moment = 0; moment < RAIN_READINGS-1; moment++) {

    // Lets set the first dot to start with
    y_len      = int(RAIN_YLEN - Rain[moment] / Rain_max * RAIN_YLEN) + RAIN_TOPY;
    y_len_next = int(RAIN_YLEN - Rain[moment+1] / Rain_max * RAIN_YLEN) + RAIN_TOPY;
   
    for (int j = 0; j <  7; j++) {
      int intermediate_bar = int((y_len_next - y_len) * j/7) + y_len;
      //Serial.println("xas: " + String(x_axes) + "; m: "  + String(Rain[moment]) + ";" + String(Rain[moment+1]) +
      //             " ylen:" + String(y_len) + ";" + String(y_len_next) + "; j:" + String(j) +
      //             "  int:" + String(intermediate_bar));
      tft.drawLine(x_axes,RAIN_TOPY + RAIN_YLEN,x_axes,intermediate_bar,bar_color);
      yield(); // give me a break
      x_axes++; // next pixel
    }  // for all intermediate steps
  } // for all 23 measurements

  // Finally print the lines for the reference values
  tft.drawLine(RAIN_TOPX
              ,int(RAIN_YLEN/3+RAIN_TOPY)
              ,RAIN_TOPY+RAIN_YLEN
              ,int(RAIN_YLEN/3+RAIN_TOPY)
              ,TFT_WHITE);   // top line
  tft.drawLine(RAIN_TOPX
              ,int(2*RAIN_YLEN/3+RAIN_TOPY)
              ,RAIN_TOPY+RAIN_YLEN             
              ,int(2*RAIN_YLEN/3+RAIN_TOPY)
              ,TFT_WHITE); // bottom line

  // and write the text for the reference value on it
  tft.setTextColor(TFT_SKYBLUE);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.drawString(String(int(lowline)) + " mm/h",5,int(2*RAIN_YLEN/3+RAIN_TOPY));
  tft.drawString(String(int(highline)) + " mm/h",5,int(RAIN_YLEN/3+RAIN_TOPY));

  tft.drawLine(105,230,135,230,TFT_MAROON);
  for (int j=0;j<(SCREEN_CHANGE_TIMEOUT / 1000);j++) {
    delay(1000);
    tft.drawPixel(105+(j*3),230,TFT_SKYBLUE);
    tft.drawPixel(105+1+(j*3),230,TFT_SKYBLUE);
  }
  tft.endWrite();
  Serial.println(F("Completed Show_Rain"));
}

void Get_CO2() {
/* *****************************************************************************
   Get_CO2

   Retrieve info from the MHZ19b sensor
 * *****************************************************************************/ 
  Serial.println(F("Executing Get_CO2")); 
  MHZ19_RESULT MHZ_Error = mhz.retrieveData();
  
  MHZ_CO2 = 0; // reset to zero to ensure we have the latest values
  MHZ_Temp = 0;

  if (MHZ_Error == MHZ19_RESULT_OK) {
    MHZ_CO2 = mhz.getCO2();
    MHZ_Temp = mhz.getTemperature();
  } else {
    Serial.print(F("Reading from MHZ19B Sensor failed; error code: "));
    Serial.println(MHZ_Error);
  }
  Serial.println(F("Completed Get_CO2"));
}

void Show_CO2() {
/* *************************************************************************************************
   Show_CO2

   Displays CO2 info on screen
 * *************************************************************************************************/
  Serial.println(F("Executing Show_CO2"));
  tft.startWrite(); 
  tft.fillScreen(TFT_BLACK);
  tft.drawLine(105,230,135,230,TFT_MAROON);
  tft.setTextSize(TEXT_SIZE_SMALL);

  for (int j=0;j<(SCREEN_CHANGE_TIMEOUT / 1000);j++) {
    Get_CO2(); 
    yield(); // give me a break
    ringMeter(MHZ_CO2, METER_MINVALUE, METER_MAXVALUE,
              METER_XPOS, METER_YPOS, METER_RADIUS,"CO2",THREECOLOR); // Draw analogue meter 
    delay(1000); 

    tft.drawPixel(105+(j*3),230,TFT_SKYBLUE);
    tft.drawPixel(105+1+(j*3),230,TFT_SKYBLUE);
  }
  tft.endWrite();
  Serial.println(F("Completed Show_CO2"));
}

void Show_Weather() {
/* *************************************************************************************************
   Show_Weather

   Gets the global saved weather info fetched from the URL and displays on the TFT screen
 * *************************************************************************************************/
  Serial.println(F("Executing Show_Weather"));

  Serial.println("Timestamp         " + WTH_timestamp);
  Serial.println("Description       " + WTH_description);
  Serial.println("Icon              " + WTH_icon); 
  Serial.println("Winddirection     " + WTH_winddirection);
  Serial.println("Airpressure       " + String(WTH_airpressure));
  Serial.println("Temperature       " + String(WTH_temperature));
  Serial.println("Windspeed         " + String(WTH_windspeed));
  Serial.println("Vochtigheid       " + String(WTH_humidity));
  Serial.println("Kans op regen     " + String(WTH_precipitation));
  Serial.println("Sunpower          " + String(WTH_sunpower));
  Serial.println("Rain last 24 hour " + String(WTH_rainFallLast24Hour));
  Serial.println("Rain last hour    " + String(WTH_rainFallLastHour));
  Serial.println("Sunrise           " + String(WTH_sunrise));
  Serial.println("Sunset            " + String(WTH_sunset)); 

  yield(); // give me a break

  tft.startWrite();
  tft.fillScreen(TFT_BLACK);

  // The name of the weather symbol is catched in the variable image
  if (WTH_icon == "a")    tft.pushImage(95,10,50,50,zonnig);
  if (WTH_icon == "j")    tft.pushImage(95,10,50,50,halfbewolkt);
  if (WTH_icon == "b" || 
      WTH_icon == "d" ||
      WTH_icon == "f")    tft.pushImage(95,10,50,50,bewolkt);
  if (WTH_icon == "c")    tft.pushImage(95,10,50,50,zwaarbewolkt);
  if (WTH_icon == "cc")   tft.pushImage(95,10,50,50,wolkennacht);
  if (WTH_icon == "g" ||
      WTH_icon == "s")    tft.pushImage(95,10,50,50,bliksem);
  if (WTH_icon == "t" ||
      WTH_icon == "u" ||
      WTH_icon == "v")    tft.pushImage(95,10,50,50,sneeuw);
  if (WTH_icon == "m")    tft.pushImage(95,10,50,50,buien);
  if (WTH_icon == "n")    tft.pushImage(95,10,50,50,mist);
  if (WTH_icon == "q")    tft.pushImage(95,10,50,50,regen);
  if (WTH_icon == "w")    tft.pushImage(95,10,50,50,hagel);

  tft.fillRect(0,68,240,32,TFT_DARKGREY);

  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setCursor(10,118);
  tft.setTextColor(TFT_GREENYELLOW);
  if (WTH_description.length() < TEXT_MAX_LENGTH) {
    tft.drawCentreString(WTH_description,120,82,TEXT_SIZE_SMALL); // Value in middle
  } else {
    int ls = WTH_description.lastIndexOf(' ', TEXT_MAX_LENGTH);
    String w1 = WTH_description.substring(0,min(ls,TEXT_MAX_LENGTH)+1);
    String w2 = WTH_description.substring(min(ls,TEXT_MAX_LENGTH)+1);
    tft.drawCentreString(w1,120,75,TEXT_SIZE_SMALL); // Value in middle
    tft.drawCentreString(w2,120,89,TEXT_SIZE_SMALL); // Value in middle
  }

  yield(); // give me a break

  // Air Pressure
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setCursor(35,108);
  tft.print("Luchtdruk");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(TEXT_SIZE_MEDIUM);
  tft.setCursor(35,118);
  tft.print(WTH_airpressure);

  // Humidity
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setCursor(155,108);
  tft.print("R/H %");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(TEXT_SIZE_MEDIUM);
  tft.setCursor(155,118);
  tft.print(WTH_humidity);

 
  tft.fillRect(0,140,240,32,TFT_DARKGREY);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setTextColor(TFT_GREENYELLOW);
  tft.drawCentreString("Zon op " + WTH_sunrise + "   Zon onder " + WTH_sunset,120,152, TEXT_SIZE_SMALL);

  // Wind direction
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setCursor(60,20);
  tft.print("Wind");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(TEXT_SIZE_MEDIUM);
  tft.setCursor(50,46);
  tft.print(WTH_winddirection);

  // Wind speed
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setCursor(155,20);
  tft.print("km/h");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(TEXT_SIZE_MEDIUM);
  tft.setCursor(155,46);
  tft.print(WTH_windspeed);

  // Temperature
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setCursor(35,178);
  tft.print("temperatuur");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(TEXT_SIZE_MEDIUM);
  tft.setCursor(35,193);
  tft.print((char)0x1f);
  tft.print(WTH_temperature);
  tft.print((char)247);

  // Sunpower
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setCursor(155,178);
  tft.print("zonkracht");
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(TEXT_SIZE_MEDIUM);
  tft.setCursor(155,193);
  tft.print(WTH_sunpower);  
  
  // bottom text
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setTextColor(TFT_MAROON);
  tft.drawCentreString("RZ Jan\'22",120,210,1);
  tft.drawCentreString("@" + WTH_timestamp + " rc" + WTH_error,120,220,1);
  tft.drawLine(105,230,135,230,TFT_MAROON);

  for (int j=0;j<(SCREEN_CHANGE_TIMEOUT / 1000);j++) {
    delay(1000);
    tft.drawPixel(105+(j*3),230,TFT_SKYBLUE);
    tft.drawPixel(105+1+(j*3),230,TFT_SKYBLUE);
  }
  tft.endWrite();

  Serial.println(F("Completed Show_Weather"));
}

String grep(String item, String payload) {
  /* *****************************************************************************
   grep

   grep one item from payload
 * *****************************************************************************/ 
  String j = payload;
  String item_extended = "\"" + item + "\":";
  int found = j.indexOf(item_extended);
  if (found == -1) return ""; // Not found. Not much use to continue
  
  j = j.substring(found);
  j = j.substring(item.length()+3,j.indexOf(",\""));
  if (j.startsWith("\""))
    return j.substring(1,j.length()-1);
  return j;
}

void Get_Weather() {
/* *************************************************************************************************
   Get_Weather

   Time to retrieve the weather info again
 * *************************************************************************************************/
  // Retrieve info from Weerlive.nl website
  Serial.println(F("Executing Get_Weather"));

  String headerDate = "";
  String payload = "";

  httpsClient.setInsecure(); // do not bother about certificate
  httpsClient.setTimeout(HTTPS_TIMEOUT_SEC * 1000);
  delay(ESTABLISH_DELAY);

  Serial.print(F("HTTPS Connecting to "));
  Serial.print(json_host);
  Serial.print(".");
  int r = 0; //retry counter
  while (( !httpsClient.connect(json_host, 443)) && (r < HTTPS_TIMEOUT_SEC)) {
    delay(1000);
    Serial.print(".");
    r++;
  }
  if (r == HTTPS_TIMEOUT_SEC) {
    Serial.println(F(" Connection failed"));
    Serial.print(F("BearSSL Last error "));
    Serial.println(httpsClient.getLastSSLError());
    return;
  }
  Serial.println(F(" Connection successfull"));

  // Get info
  httpsClient.print(String("GET ") + json_link + " HTTP/1.1\r\n" +
                    "Host: " + json_host + "\r\n" +
                    "Connection: close\r\n\r\n");

  // Receive headers
  while (httpsClient.connected()) {

    yield(); // give me a break

    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") break;

    if (line.substring(0, line.indexOf(':')) == "Date")
      headerDate = line.substring(line.indexOf(':') + 2, 99);
    if (line.substring(0, line.indexOf('/')) == "HTTP")
      WTH_error = line.substring(line.indexOf(' ') + 1, line.indexOf(' ') + 4);
  }
  
  if (WTH_error != "200") { 
    Serial.print(F("HTTP status "));
    Serial.println(WTH_error);
    return;
  }

  // Receive body. This is a JSON string. But since it is lengthy and the JSON parser requires more mem than we 
  // have, we stick to simply string grabbing
  while (httpsClient.available()) {
    yield(); // give me a break
    char c = httpsClient.read();
    payload += c;
  }
  httpsClient.stop();
  Serial.print("Received weather message with length ");
  Serial.println(payload.length());

  // Find sunrise and sunset
  payload = payload.substring(payload.indexOf("\"actual\":"));
  WTH_sunrise = grep("sunrise",payload.substring(0,200));
  WTH_sunrise = WTH_sunrise.substring(11,16);

  WTH_sunset = grep("sunset",payload.substring(0,300));
  WTH_sunset = WTH_sunset.substring(11,16);

  // Grep one station and find station vars
  payload = payload.substring(payload.indexOf(stationid));
  payload = payload.substring(0,payload.indexOf("}"));

  yield(); // give me a break
  // Find station vars
  WTH_timestamp          = grep("timestamp",payload);
  WTH_timestamp          = WTH_timestamp.substring(11,16);
  WTH_description        = grep("weatherdescription",payload);
        
  WTH_icon               = grep("iconurl",payload);
  WTH_icon               = WTH_icon.substring(WTH_icon.indexOf("30x30")+6);
  WTH_icon               = WTH_icon.substring(0,WTH_icon.indexOf(".png"));

  WTH_winddirection      = grep("winddirection",payload);
  WTH_airpressure        = grep("airpressure",payload);
  WTH_temperature        = grep("temperature",payload);
  WTH_windspeed          = grep("windspeed",payload);
  WTH_windspeed          = WTH_windspeed.substring(0,WTH_windspeed.indexOf("."));
  WTH_humidity           = grep("humidity",payload);
  WTH_precipitation      = grep("precipitation",payload);

  WTH_sunpower           = grep("sunpower",payload);
  WTH_sunpower           = WTH_sunpower.substring(0,WTH_sunpower.indexOf("."));

  WTH_rainFallLast24Hour = grep("rainFallLast24Hour",payload);
  WTH_rainFallLastHour   = grep("rainFallLastHour",payload);

  Serial.println("Completed Get_Weather");
}

void Get_Rain() {
/* *************************************************************************************************
   Get_Rain

   Time to retrieve the rain forecast
 * *************************************************************************************************/
  Serial.println(F("Executing Get_Rain"));

  int vertical_bar_position = 0; // Where to find the vertical bar in the answer from Buienradar
  int lines_read = 0;
  String line = ""; // one text line from https

  httpsClient.setInsecure(); // do not bother about certificate
  httpsClient.setTimeout(HTTPS_TIMEOUT_SEC * 1000);
  delay(ESTABLISH_DELAY);

  Serial.print(F("HTTPS Connecting to "));
  Serial.print(rain_host);
  Serial.print(F("."));

  int r = 0; //retry counter
  while (( !httpsClient.connect(rain_host, 443)) && (r < HTTPS_TIMEOUT_SEC)) {
    delay(1000);
    Serial.print(F("."));
    r++;
  }
  if (r == HTTPS_TIMEOUT_SEC) {
    Serial.println(F(" Connection failed"));
    Serial.print(F(" BearSSL Last error "));
    Serial.println(httpsClient.getLastSSLError());
    return;
  }

  Serial.println(F(" Connected successfull"));

  // Get info
  httpsClient.print(String("GET ") + rain_link1 + rain_link2 + " HTTP/1.1\r\n" +
                    "Host: " + rain_host + "\r\n" +
                    "Connection: close\r\n\r\n");

  // Receive headers
  while (httpsClient.connected()) {
    yield(); // give me a break
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r") break;

    if (line.substring(0, line.indexOf('/')) == "HTTP") {
      RAIN_error = line.substring(line.indexOf(' ') + 1, line.indexOf(' ') + 4);
    }
  }
  
  if (RAIN_error != "200") { 
    Serial.print(F("HTTP status "));
    Serial.println(RAIN_error);
    return;
  }

  // Receive body
  Rain_max = 0; // global

  while (httpsClient.available()) {
    yield(); // give me a break
    line = httpsClient.readStringUntil('\n');  //Read Line by Line
    Rain[lines_read] = 0;
    //Serial.println("Rain read ==" + line + "==");

    // Find vertical bar in line "077|20:10". Note; since end 2020 rain amount can contain decimals
    vertical_bar_position = line.indexOf('|');
    if (vertical_bar_position  > -1) {
      float radarvalue = line.substring(0, vertical_bar_position).toFloat();
      // Calculate the rain fall in mm/hour by using the values from buienradar and their 
      // logaritmic type of scale like this 10^((value -109)/32) (example: 77 = 0.1 mm/hour)
      Rain[lines_read] =  (pow(10, (radarvalue - 109) / 32.0) * 10.0) / 10.0;

      Rain_max = max(Rain[lines_read],Rain_max); // Set new max level
    }

    lines_read++;
  }
  httpsClient.stop();
 /*
  Rain[0] = 0.1;
  Rain[1] = 0.2;
  Rain[2] = 0.3;
  Rain[3] = 0.4;
  Rain[4] = 0.5;
  Rain[5] = 0.6;
  Rain[6] = 0.7;
  Rain[7] = 0.8;
  Rain[8] = 0.9;
  Rain[9] = 0.7;
  Rain[10] = 0.6;
  Rain[11] = 0.4;
  Rain[12] = 0.2;
  Rain[13] = 0.3;
  Rain[14] = 0.7;
  Rain[15] = 1;
  Rain[16] = 1.1;
  Rain[17] = 1.2;
  Rain[18] = 1.3;
  Rain[19] = 1.4;
  Rain[20] = 1.5;
  Rain[21] = 1.6;
  Rain[22] = 1.7;
  Rain[23] = 1.8;
  */
  Serial.print(F("Completed Get_Rain; Retrieved "));
  Serial.print(String(lines_read));
  Serial.print(F(" lines from "));
  Serial.println(rain_host);
}

void setup() {
/* *****************************************************************************
   Setup

   We will run this code only once, when powered on
 * *****************************************************************************/ 
 
  // Use serial port for debugging and program logic info
  Serial.begin(DEBUG_OUTPUT_BAUDRATE);
  Serial.println(); Serial.println();
  Serial.println(F("Starting..."));

  delay(ESTABLISH_DELAY); // Wait a bit to give the system time to polish the bits

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_RED);
  tft.setTextSize(TEXT_SIZE_MEDIUM);
  tft.drawCentreString("CO2 meter " + String(VERSION),METER_RADIUS,40,TEXT_SIZE_MEDIUM);
  tft.setTextColor(TFT_SKYBLUE);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.drawCentreString("github.com/idurz/RoundMeter",METER_RADIUS,74,TEXT_SIZE_SMALL);
  
  int left=35;

  tft.setCursor(left,90);
  tft.setTextSize(TEXT_SIZE_SMALL);
  tft.setTextColor(TFT_YELLOW);

  tft.print("Starting...");

  WiFiManager wifiManager; /* Local intialization in the setup routine instead of global
                              Once its business is done, there is no need to keep it around */

  WiFi.hostname(DEVICE_NAME); // Set DHCP name
  wifiManager.setAPCallback(configModeCallback); // set callback that gets called when connecting
  wifiManager.setDebugOutput(false);             // to previous WiFi fails, and enters Access Point mode

  // Fetches ssid and pass and tries to connect. If it does not connect it starts an access point
  // with the specified name and goes into a blocking loop awaiting configuration
  tft.setCursor(left,105);
  if (!wifiManager.autoConnect(DEVICE_NAME)) {
    tft.print(F("Verbinding mislukt...Herstart"));
    Serial.println(F("Connection to accesspoint did not work. Will reset myself soon"));
    delay(RESET_DELAY_ON_ERROR);  // wait a bit so user can read the message
    ESP.reset();  //reset and try again
  }
  // If we arrive here we have a network connection
  delay(ESTABLISH_DELAY); // wait a bit to get eveything finished   
  Serial.print(F("Connected to  ")); Serial.println(String(WiFi.SSID()));
  Serial.print(F("My IP address ")); Serial.println(WiFi.localIP().toString());
  Serial.print(F("My hostname   ")); Serial.println(WiFi.hostname());

  tft.print("Connected to  " + String(WiFi.SSID()));
  tft.setCursor(left,120);
  tft.print("My IP address " + WiFi.localIP().toString());
  tft.setCursor(left,135);
  tft.print("My hostname   " +WiFi.hostname());

  // start comm with MHZ-19B sensor
  sensor.begin(SENSOR_OUTPUT_BAUDRATE); 
  MHZ19_RESULT MHZ_Error = mhz.setRange(MHZ19_RANGE_3000);
  if (MHZ_Error == MHZ19_RESULT_OK) {
    Serial.println("Sensor range  0..3000");
    tft.setCursor(left,150);
    tft.print("Sensor range  0..3000");
  } else {
    Serial.print("Could not set sensor range; Error, code: ");
    Serial.println(MHZ_Error);
    tft.setCursor(left,150);
    tft.print("Sensor range  FAILED");
  }

  tft.setCursor(left,165);
  tft.print("Weerstation   " +stationid);

  tft.setCursor(left,180);
  tft.print("Regen @ " + String(rain_link2));

  delay(2500);
}

void loop() {
/* *****************************************************************************
   loop

   Keep doing this until we loose power
 * *****************************************************************************/

  // Do we need to fetch new info?
  if (json_next_get < millis()) {
    Get_Weather(); 
    json_next_get = millis() + (JSON_INTERVAL_SEC * 1000);
  } 
  if (rain_next_get < millis()) {
    Get_Rain(); 
    rain_next_get = millis() + (RAIN_INTERVAL_SEC * 1000);
  } 

  // Decide which screen to show
  screen_to_show++;
  if (screen_to_show > SCREEN_COUNT) screen_to_show = 1;

  if (screen_to_show == SCREEN_WEATHER) Show_Weather();
  if (screen_to_show == SCREEN_RAIN)    Show_Rain();
  if (screen_to_show == SCREEN_CO2)     Show_CO2();
}

/* *****************************************************************************
   FINISH
 * *****************************************************************************/
 