#include <Arduino.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <CustomWiFiManager.h>         //https://github.com/tzapu/WiFiManager

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFiManager wifiManager;  //实例化WiFiManager
  Serial.println("Start");
  wifiManager.setDebugOutput(false); //关闭Debug
  //wifiManager.setConnectTimeout(10); //设置超时
  wifiManager.setPageTitle("欢迎来到小凯的WiFi配置页");  //设置页标题

  if (!wifiManager.autoConnect("ESP8266")) {  //AP模式
    Serial.println("连接失败并超时");
    //重新设置并再试一次，或者让它进入深度睡眠状态
    delay(1000);
  }
  Serial.println("connected...^_^");
  yield();
}

void loop() {
  // put your main code here, to run repeatedly:

}
