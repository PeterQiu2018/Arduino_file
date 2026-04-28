#include <Servo.h>
const int pingPin = 8; // Trigger Pin of Ultrasonic Sensor
const int echoPin = 7; // Echo Pin of Ultrasonic Sensor
Servo myservo1;//定义舵机变量名
Servo myservo2;//定义舵机变量名
int pos = 0;
void setup()
{
  myservo1.attach(4);
  myservo2.attach(5);
  
  // Serial.begin(9600); // Starting Serial Terminal
  myservo1.write(90);
  delay(1000);
  myservo2.write(90);

}

void loop()
{

}

