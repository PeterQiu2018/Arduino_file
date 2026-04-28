#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
// #include "SH1106Wire.h"
// #include <time.h>
// #include <sys/time.h>
// #include <WiFiManager.h>

#include "weather.h"

const int I2C_DISPLAY_ADDRESS = 0x3c;  //I2c地址默认
#if defined(ESP32)
const int SDA_PIN = 32;  //引脚连接
const int SDC_PIN = 33;  //
#endif
const String UserKey = "e338421e6825458c99c68ee846f03511";
Weather weather(UserKey,"101280608");
const char* ssid     = "DY_XL";     // WIFI账户
const char* password = "dadong9c161112"; // WIFI密码
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  Serial.println("setup");
  delay(1000);
  weather.update();
  String Icon;
  Icon = weather.getTemp(true);
  Serial.println(String(Icon));
}

void loop() {
  // put your main code here, to run repeatedly:
  // display.drawString(0, 0, Current);
  // display.display();
  // delay(500);
}
