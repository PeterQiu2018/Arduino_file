#include <GyverOLED.h>
#include <Math.h>
GyverOLED<SSH1106_128x64> oled;

byte radius = 40;
static int x = 85, y = 0;
int x2 = 0, y2 = 0, xd = 0, yd = 0;
double rad;
int inputValue = 0, angle = 0, pwm = 0;
const int motorPin = 3;
# define inputPin A3

void setup() {
  Serial.begin(9600);

  pinMode(motorPin, OUTPUT);
  pinMode(inputPin, INPUT);
  
  // oled.init();
  // Wire.setClock(800000L); 
  // oled.clear();
  // oled.update();
  delay(3000);
  
}

void loop() {
  inputValue = analogRead(inputPin);
  // angle = map(inputValue,0,929,0,180);
  // for (int i = 180; i >= 0; i -= 30) {
  //   rad = angle * PI / 180.00;
  //   int x2 = int(x + cos(rad) * 40);
  //   int y2 = int(y + sin(rad) * 40 );
  //   oled.line(x, y, x2 , y2);
  //   oled.circle(x, y, radius, OLED_STROKE);
  //   for (int j = 180; j >= 0; j -= 30) {
  //     rad = j * PI / 180.00;
  //     int xd = int(x + cos(rad) * 38);
  //     int yd = int(y + sin(rad) * 38);
  //     oled.circle(xd,yd,1);
  //   }
  //   oled.update();
  //   oled.clear();
  // }
  Serial.println(inputValue);
  pwm = map(inputValue,0,929,0,255);
  Serial.println(pwm);
  Serial.println();
  analogWrite(motorPin,pwm);		// 输出PWM，占空比为 pwm/255
}

