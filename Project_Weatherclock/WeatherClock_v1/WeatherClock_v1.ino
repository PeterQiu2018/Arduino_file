#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <DHT.h>
#include <time.h>
#include "weather_icons.h"
#include "qweather_client.h"

// =====================【用户可改参数区】=====================
#define I2C_SDA_PIN 32
#define I2C_SCL_PIN 33

#define DHTPIN 5
#define DHTTYPE DHT11

const char* AP_NAME = "ESP32-WeatherClock";
const char* AP_PASS = "12345678";

const char* PREF_NS = "weathercfg";
const char* PREF_KEY_CITY = "city";
const char* DEFAULT_CITY = "hangzhou";

const unsigned long PAGE_SWITCH_MS = 8000;        // 页面切换
const unsigned long DHT_REFRESH_MS = 2000;        // DHT刷新
const unsigned long WEATHER_REFRESH_MS = 600000;  // 天气刷新
// ===========================================================

// 你这块从照片看是 SH1106 模组（1.3" 常见），用错 SSD1306 驱动会在右侧出现花屏/竖条
// 这里改成 U8G2_R2，让整屏 180° 倒转，适合“上下反过来安装”的情况。
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);
DHT dht(DHTPIN, DHTTYPE);
Preferences prefs;

QWeatherClient weatherClient;
String cityParamValue = DEFAULT_CITY;

float dhtTemp = NAN;
float dhtHum = NAN;

unsigned long lastDhtReadMs = 0;
unsigned long lastWeatherUpdateMs = 0;
unsigned long lastPageSwitchMs = 0;
int currentPage = 0;  // 0=当前天气 1=三日预报 2=大字时间

String twoDigits(int n) {
  if (n < 10) return "0" + String(n);
  return String(n);
}

void drawCentered(int y, const String& text, const uint8_t* font) {
  u8g2.setFont(font);
  int w = u8g2.getStrWidth(text.c_str());
  int x = (128 - w) / 2;
  if (x < 0) x = 0;
  u8g2.drawStr(x, y, text.c_str());
}

void syncTimeByNTP() {
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp.tencent.com", "pool.ntp.org");
  struct tm timeinfo;
  for (int i = 0; i < 25; i++) {
    if (getLocalTime(&timeinfo, 200)) return;
    delay(200);
  }
}

void readDhtIfDue() {
  if (millis() - lastDhtReadMs < DHT_REFRESH_MS) return;
  lastDhtReadMs = millis();

  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) dhtTemp = t;
  if (!isnan(h)) dhtHum = h;
}

void updateWeatherIfDue(bool force = false) {
  if (!force && (millis() - lastWeatherUpdateMs < WEATHER_REFRESH_MS)) return;
  lastWeatherUpdateMs = millis();
  if (!weatherClient.update()) {
    Serial.printf("[QW] update failed: %s\n", weatherClient.getLastError().c_str());
  }
}

void switchPageIfDue() {
  if (millis() - lastPageSwitchMs < PAGE_SWITCH_MS) return;
  lastPageSwitchMs = millis();
  currentPage = (currentPage + 1) % 3;
}

// -------------------- 页面绘制（尽量贴近旧布局） --------------------
String ellipsizeToWidth(const String& s, int maxW) {
  if (u8g2.getStrWidth(s.c_str()) <= maxW) return s;
  String out = s;
  while (out.length() > 0) {
    out.remove(out.length() - 1);
    String t = out + "...";
    if (u8g2.getStrWidth(t.c_str()) <= maxW) return t;
  }
  return "";
}

void drawBottomBarTimeDHT(const String& leftText = "") {
  struct tm t;
  u8g2.setFont(u8g2_font_6x10_tf);

  // 右下：DHT温湿度
  String dhtLine;
  if (isnan(dhtTemp) || isnan(dhtHum)) dhtLine = "--C --%";
  else dhtLine = String((int)dhtTemp) + "C " + String((int)dhtHum) + "%";
  int wRight = u8g2.getStrWidth(dhtLine.c_str());
  int xRight = 126 - wRight;
  u8g2.drawStr(xRight, 63, dhtLine.c_str());

  // 左下：优先传入文本；否则显示当前时间
  String left = leftText;
  if (left.length() == 0) {
    if (getLocalTime(&t, 5)) left = twoDigits(t.tm_hour) + ":" + twoDigits(t.tm_min) + ":" + twoDigits(t.tm_sec);
    else left = "--:--:--";
  }

  int maxLeftW = xRight - 6; // 留一点间距
  if (maxLeftW < 8) maxLeftW = 8;
  left = ellipsizeToWidth(left, maxLeftW);
  u8g2.drawStr(2, 63, left.c_str());

  u8g2.drawHLine(0, 52, 128);
}

void drawPageCurrentWeather() {
  CurrentWeather cw = weatherClient.getCurrent();

  // 第一行：左上真实天气图标 + 右温度（避免48x48图标与温度重叠）
  drawWeatherIcon(-6, -9, cw.code);

  u8g2.setFont(u8g2_font_logisoso24_tf);
  String temp = String(cw.temp);
  u8g2.drawStr(60, 24, temp.c_str());

  // 大字体可能不含“℃”，改为手动画“°”+“C”
  int tempW = u8g2.getStrWidth(temp.c_str());
  int degX = 60 + tempW + 2;
  u8g2.drawCircle(degX, 6, 2, U8G2_DRAW_ALL);
  u8g2.setFont(u8g2_font_logisoso24_tf);
  u8g2.drawStr(degX + 5, 24, "C");

  // 第二行：中间固定 "|"，左边天气，右边风速
  u8g2.setFont(u8g2_font_7x13_tf);
  const int midX = 64;
  const int barY = 38;
  const int gap = 4;

  String leftText = cw.text;
  String rightText = "Wind:" + String(cw.windSpeed, 1);

  int barW = u8g2.getStrWidth("|");
  int leftW = u8g2.getStrWidth(leftText.c_str());
  int rightW = u8g2.getStrWidth(rightText.c_str());

  int leftX = midX - gap - barW / 2 - leftW;
  int rightX = midX + gap + (barW - barW / 2);

  if (leftX < 0) leftX = 0;
  if (rightX + rightW > 128) rightX = 128 - rightW;

  u8g2.drawStr(leftX, barY, leftText.c_str());
  u8g2.drawStr(midX - barW / 2, barY, "|");
  u8g2.drawStr(rightX, barY, rightText.c_str());

  // 第三行：城市 + 体感湿度（字体调小）
  u8g2.setFont(u8g2_font_5x8_tf);
  String line3 = weatherClient.getCity() + "  FL:" + String(cw.feelsLike) + "C RH:" + String(cw.humidity) + "%";
  drawCentered(49, line3, u8g2_font_5x8_tf);

  drawBottomBarTimeDHT();
}

void drawForecastCell(int x, const ForecastDay& d) {
  // 参考旧版列布局 x / x+44 / x+88
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(x + 2, 12, d.date.c_str());

  drawWeatherIcon(x - 7, 0, d.code);

  u8g2.setFont(u8g2_font_6x10_tf);
  String mm = String(d.tempMin) + "-" + String(d.tempMax)+ "C";
  u8g2.drawStr(x + 2, 46, mm.c_str());
}

void drawPageForecast3() {
  const ForecastDay* f = weatherClient.getForecast3();

  drawForecastCell(0, f[0]);
  drawForecastCell(44, f[1]);
  drawForecastCell(88, f[2]);

  // 竖线分隔
  u8g2.drawVLine(42, 0, 52);
  u8g2.drawVLine(86, 0, 52);

  drawBottomBarTimeDHT();
}

void drawPageBigTime() {
  struct tm t;
  if (getLocalTime(&t, 10)) {
    String date = String(t.tm_year + 1900) + "-" + twoDigits(t.tm_mon + 1) + "-" + twoDigits(t.tm_mday);
    drawCentered(12, date, u8g2_font_6x10_tf);

    String hhmmss = twoDigits(t.tm_hour) + ":" + twoDigits(t.tm_min) + ":" + twoDigits(t.tm_sec);
    drawCentered(44, hhmmss, u8g2_font_logisoso22_tf);
  } else {
    drawCentered(34, "NTP Syncing...", u8g2_font_7x13_tf);
  }

  CurrentWeather cw = weatherClient.getCurrent();
  String weatherText = cw.text;
  if (weatherText.length() == 0) weatherText = "N/A";
  drawBottomBarTimeDHT(weatherText);
}

void drawScreen() {
  u8g2.clearBuffer();

  if (currentPage == 0) drawPageCurrentWeather();
  else if (currentPage == 1) drawPageForecast3();
  else drawPageBigTime();

  u8g2.sendBuffer();
}

void setupWiFiAndCity() {
  WiFi.mode(WIFI_STA);

  WiFiManager wm;
  WiFiManagerParameter customCity("city", "City", cityParamValue.c_str(), 32);
  wm.addParameter(&customCity);

  wm.setConfigPortalTimeout(180);
  bool ok = wm.autoConnect(AP_NAME, AP_PASS);  // 连不上缓存就进AP配网

  String newCity = String(customCity.getValue());
  newCity.trim();
  if (newCity.length() == 0) newCity = DEFAULT_CITY;

  cityParamValue = newCity;
  prefs.putString(PREF_KEY_CITY, cityParamValue);

  if (!ok) {
    delay(1000);
    ESP.restart();
  }

  // 连上后保留10秒配置入口
  wm.setConfigPortalBlocking(false);
  wm.startConfigPortal(AP_NAME, AP_PASS);

  unsigned long t0 = millis();
  while (millis() - t0 < 15000) {
    wm.process();

    u8g2.clearBuffer();
    drawCentered(24, "Config portal 15s", u8g2_font_6x10_tf);
    drawCentered(38, String("AP: ") + AP_NAME, u8g2_font_6x10_tf);
    u8g2.sendBuffer();
    delay(30);
  }
  wm.stopConfigPortal();

  // 再读一次，保留10秒内修改
  String city2 = String(customCity.getValue());
  city2.trim();
  if (city2.length() > 0 && city2 != cityParamValue) {
    cityParamValue = city2;
    prefs.putString(PREF_KEY_CITY, cityParamValue);
  }

  weatherClient.setCity(cityParamValue);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  u8g2.begin();
  u8g2.enableUTF8Print();

  dht.begin();

  prefs.begin(PREF_NS, false);
  cityParamValue = prefs.getString(PREF_KEY_CITY, DEFAULT_CITY);

  u8g2.clearBuffer();
  drawCentered(30, "Booting...", u8g2_font_7x13_tf);
  u8g2.sendBuffer();

  setupWiFiAndCity();

  syncTimeByNTP();

  weatherClient.begin(cityParamValue);
  updateWeatherIfDue(true);

  lastPageSwitchMs = millis();
  lastDhtReadMs = 0;
  lastWeatherUpdateMs = 0;
}

void loop() {
  readDhtIfDue();
  updateWeatherIfDue();
  switchPageIfDue();
  drawScreen();

  delay(50);
}
