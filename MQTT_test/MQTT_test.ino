#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#define LED 0

// char* ssid = "***";  // WIFI账户
// char* password = "***";           // WIFI密码
// WiFi

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *led_topic = "f2a4a7fe22b14bec/relay";
const char *led_lastup = "f2a4a7fe22b14bec/relay_lastup";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
bool ledState = false;
const long utcOffsetInSeconds = 28800;


WiFiClient espClient;
PubSubClient client(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "cn.pool.ntp.org", utcOffsetInSeconds);

void setup() {
    // Set software serial baud to 115200;
  Serial.begin(115200);
  WiFiManager wifiManager;  //实例化WiFiManager
  Serial.println("Start");
  wifiManager.setDebugOutput(false); //关闭Debug
  //wifiManager.setConnectTimeout(10); //设置超时

  if (!wifiManager.autoConnect("ESP8266")) {  //AP模式
    Serial.println("连接失败并超时");
    //重新设置并再试一次，或者让它进入深度睡眠状态
    delay(1000);
  }
  Serial.println("connected...^_^");
  yield();
  Serial.println("Connected to the WiFi network");

  // Setting LED pin as output
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);  // Turn off the LED initially

  timeClient.begin();

  // Connecting to an MQTT broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
      String client_id = "esp8266-client-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("Public EMQX MQTT broker connected");
      } else {
          Serial.print("Failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }

    // Publish and subscribe
    client.publish(led_topic, "IoT device shenzhen up");
    client.subscribe(led_topic);
    for (int i = 0; i < 5; i++){
      digitalWrite(LED, LOW); // Turn off the LED
      delay(200);
      digitalWrite(LED, HIGH); // Turn off the LED
      delay(200);
      digitalWrite(LED, LOW); // Turn off the LED
    }
}

void callback(char *led_topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(led_topic);
    Serial.print("Message: ");
    String message;
    for (int i = 0; i < length; i++) {
        message += (char) payload[i];  // Convert *byte to string
    }
    Serial.print(message);
    if (message == "on" && !ledState) {
        digitalWrite(LED, HIGH);  // Turn on the LED
        ledState = true;
    }
    if (message == "off" && ledState) {
        digitalWrite(LED, LOW); // Turn off the LED
        ledState = false;
    }
    if (message == "blink") {
        digitalWrite(LED, LOW); // Turn off the LED
        delay(500);
        digitalWrite(LED, HIGH); // Turn off the LED
        delay(500);
        digitalWrite(LED, LOW); // Turn off the LED
        ledState = false;
    }
    Serial.println();
    Serial.println("-----------------------");
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Reconnecting to MQTT broker...");
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
        Serial.println("Reconnected to MQTT broker.");
        client.subscribe(led_topic);
    } else {
        Serial.print("Failed to reconnect to MQTT broker, rc=");
        Serial.print(client.state());
        Serial.println("Retrying in 5 seconds.");
        delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  timeClient.update();
  if (timeClient.getMinutes() % 5 == 0) {
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime); 
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    int currentYear = ptm->tm_year+1900;
    String FormatTime = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay)+" " + timeClient.getFormattedTime();
    char lastup_time[64];
    strcpy(lastup_time, FormatTime.c_str());
    client.publish(led_lastup, lastup_time);
  }
  client.loop();
}