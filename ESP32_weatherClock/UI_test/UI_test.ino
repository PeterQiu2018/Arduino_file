#include <Arduino.h>
#include<String.h>

#include "SH1106Wire.h"
#include "OLEDDisplayUi.h"
#include "weatherIcon_sheffield.h"
// #include <DHT.h>

#define DHTPIN 5   //DHT11 DATA 
#define DHTTYPE DHT11  //select DHT type
// DHT dht(DHTPIN, DHTTYPE);

const int I2C_DISPLAY_ADDRESS = 0x3c;  //I2c地址默认
long timeSinceLastWUpdate = 0;    //上次更新后的时间
long timeSinceLastCurrUpdate = 0;   //上次天气更新后的时间
const int UPDATE_INTERVAL_SECS = 5 * 60; // 5分钟更新一次天气
bool readyForWeatherUpdate = false; // 天气更新标志
#if defined(ESP32)
const int SDA_PIN = 32;  //引脚连接
const int SDC_PIN = 33;  //
#endif

const char* Icon[]={gImage_01,gImage_02,gImage_03,gImage_04,gImage_09,gImage_10,gImage_11,gImage_13,gImage_50};

SH1106Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);   // 1.3寸用这个
OLEDDisplayUi   ui( &display );


void Xbm01(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[0]);
}

void Xbm02(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[1]);
}

void Xbm03(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[2]);
}
void Xbm04(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[3]);
}
void Xbm09(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[4]);
}
void Xbm10(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[5]);
}
void Xbm11(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[6]);
}
void Xbm13(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[7]);
}
void Xbm50(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->clear();
  display->drawXbm(x , y , 64, 64, Icon[8]);
}
FrameCallback frames[] = { Xbm01,Xbm02,Xbm03,Xbm04,Xbm09,Xbm10,Xbm11,Xbm13,Xbm50 };
int numberOfFrames = 9;

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {   //绘图页眉覆盖
  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  // float temperature = dht.readTemperature();
  // float humidity = dht.readHumidity();
  // String Current=String(temperature,0)+"C | "+String(humidity,0)+"%";
  // display->drawString(100, 54, Current);
  display->drawHorizontalLine(0, 52, 128);
}

OverlayCallback overlays[] = { drawHeaderOverlay }; //覆盖回调函数
int numberOfOverlays = 1;  //覆盖数

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");
  
  // 屏幕初始化
  display.init();
  // display.flipScreenVertically(); //屏幕翻转
  display.setContrast(255); //屏幕亮度
  delay(1000);
  display.clear();
  display.display();
  ui.setTargetFPS(30);  //刷新频率
  ui.setIndicatorPosition(BOTTOM);
  ui.setIndicatorDirection(LEFT_RIGHT);
  ui.setFrameAnimation(SLIDE_LEFT);
  ui.setFrames(frames, numberOfFrames); // 设置框架
  ui.setTimePerFrame(5000); //设置切换时间
  ui.setOverlays(overlays, numberOfOverlays); //设置覆盖
  // UI负责初始化显示
  ui.init();
  display.flipScreenVertically(); //屏幕翻转

  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() - timeSinceLastWUpdate > (1000L * UPDATE_INTERVAL_SECS)) { //屏幕刷新
    timeSinceLastWUpdate = millis();
  }
  int remainingTimeBudget = ui.update(); //剩余时间预算

  if (remainingTimeBudget > 0) {
    //你可以在这里工作如果你低于你的时间预算。
    delay(remainingTimeBudget);
  }
}
