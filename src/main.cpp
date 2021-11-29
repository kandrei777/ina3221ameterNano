// stty -F /dev/ttyUSB0 cs8 115200 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts raw

#define VERSION_STR "Ver: 1"

#include <Arduino.h>
#include <Wire.h>

// SDL_Arduino_INA3221 Library: https://github.com/switchdoclabs/SDL_Arduino_INA3221
#include "SDL_Arduino_INA3221.h"
// Alternative: https://github.com/beast-devices/Arduino-INA3221/blob/main/Beastdevices_INA3221.h

#define DISPLAY_ADA

#ifdef DISPLAY_ADA
// DISPLAY https://github.com/adafruit/Adafruit_SSD1306/blob/master/examples/ssd1306_128x32_i2c/ssd1306_128x32_i2c.ino
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif // DISPLAY_ADA

SDL_Arduino_INA3221 ina3221;

// the three channels of the INA3221 named for SunAirPlus Solar Power Controller channels (www.switchdoc.com)
#define CHANNELS 3
#define READ_DELAY 200
// How often to refresh the display. Refresh time ms = READ_DELAY * DISPLAY_EVERY
#define DISPLAY_EVERY 5

// Struct to describe channel data.
struct channel_t
{
  uint8_t options;
  double capacity;
  float avgI_sum;
  float avgI_min;
  float avgI_max;
} channels[3] = {}; // Set all zero.

// Options bits (bit numbers 0..8)
// Flag: Do not read the channel.
#define OPT_B_DISABLED 0
// Flag: Log data to the Serial.
#define OPT_B_LOG 1

boolean sUseDisplay; // Is the a display found.
uint8_t sCycle;      // Measure counter before display.
uint32_t sLastMeasureTime;
uint32_t sNextMeasureTime;

void setup(void)
{
  // Init serial
  Serial.begin(115200);
  Serial.println(F("# " VERSION_STR));

  // Init display
#ifdef DISPLAY_ADA
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x32
    Serial.println(F("# Error: SSD1306 allocation failed"));
    sUseDisplay = false;
  }
  else
  {
    sUseDisplay = true;
    oled.clearDisplay();
    oled.setTextSize(2);              // Normal 1:1 pixel scale
    oled.setTextColor(SSD1306_WHITE); // Draw white text
    oled.setCursor(0, 0);             // Start at top-left corner
    oled.println(F("3CH AMeter"));
    oled.print(F(VERSION_STR));
    oled.display();
    oled.setTextSize(1);
  }
#endif // DISPLAY_ADA

  // Init ina
  ina3221.begin();
}

void measure()
{

  uint32_t Time = millis();
  uint32_t passed = Time - sLastMeasureTime;
  sLastMeasureTime = Time;
  sNextMeasureTime = Time + READ_DELAY; // calculate now to exculde calculation delay.

  for (uint8_t i = 0; i < CHANNELS; i++)
  {
    auto ch = &channels[i];
    if (bit_is_set(ch->options, OPT_B_DISABLED))
    {
      continue;
    }
    // Read the channel current.
    float current_mA = ina3221.getCurrent_mA(i + 1);
    // Add the channel into dumping if non zero value.
    if (bit_is_clear(ch->options, OPT_B_LOG))
    {
      if (current_mA == 0.0)
      {
        continue; // already all zero.
      }
      else
      {
        ch->options |= _BV(OPT_B_LOG);
      }
    }
    // Calculate data.
    if (sCycle)
    {
      if (current_mA > ch->avgI_max)
      {
        ch->avgI_max = current_mA;
      }
      else if (current_mA < ch->avgI_min)
      {
        ch->avgI_min = current_mA;
      }
    }
    else
    {
      ch->avgI_min = ch->avgI_max = current_mA;
      ch->avgI_sum = 0;
    }

    ch->capacity += (double)(current_mA * passed) / (3600.0 * 1000.0);
    ch->avgI_sum += current_mA;
  } // Measure channels

  sCycle++;

  // Check if it is the time to dump.
  if (sCycle >= DISPLAY_EVERY)
  {

#ifdef DISPLAY_ADA
    if (sUseDisplay)
    {
      oled.clearDisplay();
      oled.setCursor(0, 0);
    }
#endif // DISPLAY_ADA

    for (uint8_t i = 0; i < CHANNELS; i++)
    {
      auto ch = &channels[i];
      float i_mA = ch->avgI_sum / sCycle;

      if (bit_is_set(ch->options, OPT_B_LOG))
      {
        // Out serial
        Serial.print(Time / 1000.0, 3);
        Serial.print(F(" CH"));
        Serial.print(i + 1);
        Serial.print(F(": Iavg: "));
        Serial.print(i_mA);
        Serial.print(F(" Imin: "));
        Serial.print(ch->avgI_min);
        Serial.print(F(" Imax: "));
        Serial.print(ch->avgI_max);
        Serial.print(F(" mA"));
        Serial.print(F(", Cap: "));
        Serial.print(ch->capacity);
        Serial.println(F(" mAh"));
      }

#ifdef DISPLAY_ADA
      if (sUseDisplay)
      {
        char buff[10];
        oled.print(dtostrf(i_mA, 6, 1, buff));
        oled.print(F(" mA "));
        oled.print(dtostrf(ch->capacity, 7, 1, buff));
        oled.println(F(" mAh"));
      }
#endif // DISPLAY_ADA

    } // Dump channels

#ifdef DISPLAY_ADA
    if (sUseDisplay)
    {
      oled.print(F("Time: "));
      oled.print(Time / 1000);
      oled.display();
    }  // Display
#endif // DISPLAY_ADA

    sCycle = 0;
  } // Display result.
}

void loop(void)
{
  measure();
  delay(sNextMeasureTime - millis());
}
