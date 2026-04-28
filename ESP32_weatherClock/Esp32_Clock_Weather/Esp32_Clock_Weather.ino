#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include <time.h>
#include <sys/time.h>
#include <WiFiManager.h>

#include <U8g2lib.h>
#include <Wire.h>

#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"
#include "ESP32_Heweather.h"
#include "DHT.h"
#include <U8g2lib.h>
#include <WiFi.h>
#include <time.h>
#include <DHT.h>
// #include "Weather.h"

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

U8G2_SH1106_128X64_NONAME_1_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

WeatherNow weatherNow;
WeatherForecast WeatherForecast;

const char* WDAY_NAMES[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
time_t now;
bool readyForWeatherUpdate = false;

// 函数声明（修正参数类型）
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void updateDatas(OLEDDisplay *display);
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawweatherNow(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyForWeatherUpdate();
const unsigned char* Match_icon(int Icon);

// 页面切换控制变量
int currentFrame = 0;
unsigned long lastChangeTime = 0;
const unsigned long FRAME_DURATION = 15000; // 15秒切换

void setup() {
  Serial.begin(115200);
  Serial.println();
  dht.begin(); // DHT11初始化

  // 屏幕初始化
  Wire.begin(32, 33);
  display.begin();
  display.setFont(u8g2_font_unifont_t_chinese2); // 设置中文字体
  display.enableUTF8Print();
  display.setFlipMode(1); // 屏幕翻转
  display.setContrast(255); // 屏幕亮度
  display.clearBuffer();
  display.sendBuffer();
  delay(200);

  // 连接WiFi和NTP等初始化代码...
  // (保留您原有的网络连接和时间同步代码)

  // 首次获取天气
  updateDatas(&display);
}

void loop() {
  unsigned long currentMillis = millis();
  
  // 每15秒切换页面
  if (currentMillis - lastChangeTime >= FRAME_DURATION) {
    lastChangeTime = currentMillis;
    currentFrame = (currentFrame + 1) % 3; // 在0-2间循环
  }

  display.clearBuffer();
  
  // 根据当前帧调用对应绘制函数
  switch (currentFrame) {
    case 0:
      drawDateTime(&display, nullptr, 0, 0);
      break;
    case 1:
      drawweatherNow(&display, nullptr, 0, 0);
      break;
    case 2:
      drawForecast(&display, nullptr, 0, 0);
      break;
  }
  
  // 绘制顶部状态栏（覆盖层）
  drawHeaderOverlay(&display, nullptr);
  
  display.sendBuffer();
  delay(100); // 降低CPU占用

  // 天气更新逻辑（保留原有逻辑）
  if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) {
    setReadyForWeatherUpdate();
    lastDownloadUpdate = millis();
  }
  
  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateDatas(&display);
  }
}

// ========== 页面绘制函数实现 ==========

void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(u8g2_font_helvB12_tf);
  sprintf_P(buff, PSTR("%04d-%02d-%02d  %s"), 
            timeInfo->tm_year + 1900, 
            timeInfo->tm_mon + 1, 
            timeInfo->tm_mday, 
            WDAY_NAMES[timeInfo->tm_wday]);
  display->drawString(64 + x, 5 + y, String(buff));
  
  display->setFont(u8g2_font_logisoso28_tf);
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), 
            timeInfo->tm_hour, 
            timeInfo->tm_min, 
            timeInfo->tm_sec);
  display->drawString(64 + x, 22 + y, String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawweatherNow(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setFont(u8g2_font_helvB08_tf);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, 
                      weatherNow.getWeatherText() + 
                      "|Wind: " + 
                      weatherNow.getWindScale() + "  ");
  
  display->setFont(u8g2_font_logisoso24_tf);
  display->drawString(64 + x, 54 + y, weatherNow.getTemp() + "°C");
}

void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  drawForecastDetails(display, x + 10, y, 0);
  drawForecastDetails(display, x + 44, y, 1);
  drawForecastDetails(display, x + 78, y, 2);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  time_t time = WeatherForecast.getTime(dayIndex);
  struct tm* t = localtime(&time);
  display->setFont(u8g2_font_helvB08_tf);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(x + 25, y + 5, WDAY_NAMES[t->tm_wday]);
  
  display->drawXbm(x + 5, y + 12, 32, 32, 
                  Match_icon(WeatherForecast.getIcon(dayIndex)));
  
  display->setFont(u8g2_font_helvB08_tf);
  display->drawString(x + 25, y + 45, 
                     WeatherForecast.getTempMin(dayIndex) + 
                     "/" + 
                     WeatherForecast.getTempMax(dayIndex));
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

// ========== 辅助函数实现 ==========

const unsigned char* Match_icon(int Icon) {
  const unsigned char* I;
  if (Icon == 100) {
    I = gImage_01;
  } else if (Icon == 103 || Icon == 104) {
    I = gImage_02;
  } else if (Icon == 102) {
    I = gImage_03;
  } else if (Icon == 101) {
    I = gImage_04;
  } else if (Icon >=305 && Icon <= 310) {
    I = gImage_09;
  } else if (Icon == 300 || Icon ==301) {
    I = gImage_10;
  } else if (Icon >=304 && Icon <=304 ) {
    I = gImage_11;
  } else if (Icon == 400) {
    I = gImage_13;
  } else if (Icon == 500) {
    I = gImage_50;
  } else {
    I = gImage_default; // 使用默认图标
  }
  return I;
}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  display->clearBuffer();
  display->setFont(u8g2_font_helvB10_tf);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64, 10, label);
  display->drawRect(10, 18, 108, 12);
  display->fillRect(12, 20, 104 * percentage / 100, 8);
  display->sendBuffer();
}

void updateDatas(OLEDDisplay *display) {
  drawProgress(display, 33, "Updating weather...");
  weatherNow.update();
  
  drawProgress(display, 66, "Updating forecasts...");
  WeatherForecast.update();
  
  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done...");
  delay(200);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  // 顶部状态栏实现（WiFi信号、电池电量等）
  // 保留您原有的状态栏实现
}

void setReadyForWeatherUpdate() {
  readyForWeatherUpdate = true;
}