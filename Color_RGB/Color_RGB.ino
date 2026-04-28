#include <FastLED.h>
#define LED_PIN     7
#define Button_PIN 6
#define analogInPin  A0
#define NUM_LEDS    20
#define CHIPSET     WS2812
#define COLOR_ORDER GRB
#define TEMPERATURE_2 Tungsten100W
CRGB leds[NUM_LEDS];
int BRIGHTNESS = 10;
int hVal = 255;
int mode_state = 0; 
int DELAYTIME = 0;
int sensorValue = 0;
int outputValue = 0;
void loop()
{
  if (digitalRead(Button_PIN) == LOW) // 按钮在按下后是LOW的状态
  {
    switch (mode_state) {
      case 0:
        mode_state++;
        break;
      case 1:
        mode_state = 0;
        break;
    }
    delay(500);
  }
  sensorValue = analogRead(analogInPin);
  outputValue = map(sensorValue, 0, 1023, 255, 0);
  switch (mode_state) {
    case 0:         //色调控制
      DELAYTIME = outputValue;
      break;
    case 1:         //亮度控制
      BRIGHTNESS = outputValue;
      break;
  }
  if (hVal == 256) {
    hVal = 0;
  }
  hVal++;
  CHSV myHSVcolor(hVal, 255, BRIGHTNESS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, myHSVcolor);
  delay(DELAYTIME);
  FastLED.show();
}

void setup() {
  FastLED.clear();
  pinMode(Button_PIN, INPUT_PULLUP);
  delay( 1000 ); // power-up safety delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(127);
}
