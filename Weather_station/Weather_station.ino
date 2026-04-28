#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <WiFiAP.h>

WiFiAPClass WiFiAP;
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";
uint8_t x=1;
IPAddress local_IP(192, 168, x, 1);  
IPAddress gateway(192, 168, x, 1);
IPAddress subnet(255, 255, 255, 0);
//Variables to save values from HTML form
String ssid;
String pass;

String HTML = "<!DOCTYPE html> <html> <head> <meta charset=\"utf-8\"> <title>interface</title></head><body><div><h1>ESP Wi-Fi Manager</h1></div><div><div><div><form action=\"/\" method=\"POST\"><p><label for=\"ssid\">SSID</label><input type=\"text\" id =\"ssid\" name=ssid><br><label for=\"pass\">Password</label> <input type=\"text\" id =\"pass\" name=pass><a href=\"/26/off\"><button >Submit</button></a></p></form></div></div></div></body></html>";

// Initialize WiFi
void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  // Connect to Wi-Fi network with SSID and password
  Serial.println("Setting AP (Access Point)");
  // NULL sets an open Access Point
  WiFiAP.softAPConfig(local_IP,gateway,subnet);
  WiFiAP.softAP("ESP_test", "dy123456");

  Serial.println("Server started");


  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", HTML);
  });
  
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      const AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST ssid value
        if (p->name() == PARAM_INPUT_1) {
          ssid = p->value().c_str();
          Serial.print("SSID set to: ");
          Serial.println(ssid);
        }
        // HTTP POST pass value
        if (p->name() == PARAM_INPUT_2) {
          pass = p->value().c_str();
          Serial.print("Password set to: ");
          Serial.println(pass);
        }
        //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    delay(3000);
  });
  server.begin();
  
}

void loop() {

}