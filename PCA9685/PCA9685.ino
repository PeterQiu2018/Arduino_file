#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// SG90-->400-2500=0-180   MG996R-->400-2500=0-120
#define SERVO_FREQ 50  // Analog servos run at ~50 Hz updates
int angle1=0;
int angle2=0;
int PWM_value1=0;
int PWM_value2=0;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Servo test!");
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

  pwm.writeMicroseconds(0, 1350);  //0°
  angle1 = -85;
  PWM_value1 = map(angle1, -90, 90, 500, 2450);
  pwm.writeMicroseconds(1, PWM_value1);
  angle2 = -30;
  PWM_value2 = map(angle2, 90, -90, 600, 2500);
  pwm.writeMicroseconds(2, PWM_value2);
  delay(2000);
}

void loop() {

  //SG90-->600-2500 = -90<=>90   MG996R-->500-2450=0-120

  // for (int i = 500; i < 2450; i++) {
  //   pwm.writeMicroseconds(0, i);
  //   Serial.println(i);
  // }

  angle2 = 90;
  PWM_value2 = map(angle2, 90, -90, 600, 2500);
  pwm.writeMicroseconds(2, PWM_value2);
}
