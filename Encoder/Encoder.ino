#include <FastLED.h>
#define LED_PIN     7
#define Button_PIN 5
#define pin_A 4
#define pin_B 3
#define NUM_LEDS    8
#define CHIPSET     WS2812
#define COLOR_ORDER GRB
#define TEMPERATURE_2 Tungsten100W
int Value = 0;    // 编码器输出值
int Step = 5;    // 定义每次旋转的步进值

unsigned long currentTime;
unsigned long loopTime;

unsigned char encoder_A;
unsigned char encoder_B;
unsigned char encoder_A_prev=0;

CRGB leds[NUM_LEDS];
int BRIGHTNESS = 10;
int hVal = 255;
int mode_state = 0; 
// int DELAYTIME = 50;
int sensorValue = 0;
int outputValue = 0;

void setup()  {

  Serial.begin(115200);

  pinMode(pin_A, INPUT_PULLUP); 
  pinMode(pin_B, INPUT_PULLUP);

  currentTime = millis();
  loopTime = currentTime;

  FastLED.clear();
  pinMode(Button_PIN, INPUT_PULLUP);
  delay(10); // power-up safety delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  FastLED.setBrightness(127);
  FastLED.show();
  delay(500); // power-up safety delay
}

void loop()  {
  // 获取经过的时间
  currentTime = millis();
  if(currentTime >= (loopTime + 5)){
    // 5ms = 200Hz  
    encoder_A = digitalRead(pin_A);    // 读取编码器值
    encoder_B = digitalRead(pin_B);   
    if((!encoder_A) && (encoder_A_prev)){
      if(encoder_B) {
        if(Value + Step <= 255) Value += Step;               
      } else {
        if(Value - Step >= 1) Value -= Step;               
      }   
    }
    if((encoder_A) && (!encoder_A_prev)){
      if(!encoder_B) {
        if(Value + Step <= 255) Value += Step;               
      } else {     
        if(Value - Step >= 1) Value -= Step;               
      }   
    }
    encoder_A_prev = encoder_A;
    loopTime = currentTime;  
  }
  BRIGHTNESS = Value;
  if (digitalRead(Button_PIN) == LOW) // 按钮在按下后是LOW的状态
  {
    Value = 0;
  }
  if (hVal == 256) {
    hVal = 0;
  }
  hVal++;
  CHSV myHSVcolor(hVal, 255, BRIGHTNESS);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, myHSVcolor);
  // delay(DELAYTIME);
  FastLED.show();
  Serial.println(Value);
  // delay(100);
  // 其他补充的代码，可以从这里开始
}