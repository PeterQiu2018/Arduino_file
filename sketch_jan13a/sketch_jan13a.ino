#include <FastLED.h>

#define CHIPSET     WS2812
#define COLOR_ORDER GRB
#define TEMPERATURE_2 Tungsten100W
CRGB leds[12];
int hVal=0;
int BRIGHTNESS=0;
unsigned long currentTime;

void setup() {
  // put your setup code here, to run once:
  FastLED.clear();
  FastLED.addLeds<CHIPSET, 2, COLOR_ORDER>(leds, 12).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(127);
  FastLED.show();
}

void loop() {
  // put your main code here, to run repeatedly:
  currentTime = millis();
  if (hVal == 256) {
    hVal = 0;
  }
  if (currentTime % 20 == 1){
    // hVal++;
    CHSV myHSVcolor(hVal, 255, BRIGHTNESS);
    FastLED.setBrightness(BRIGHTNESS);
    fill_solid(leds, 12, myHSVcolor);
  }
  FastLED.show();
}
