#include "qweather_client.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoUZlib.h>
#include <time.h>

#include "mbedtls/base64.h"
#include <Ed25519.h>

// ===== QWeather 配置 =====
static const char* QW_API_HOST = "m36e48jcjm.re.qweatherapi.com";
static const char* QW_KID = "TFWG39JNJ8";
static const char* QW_SUB = "2D88DP9BBU";
static const uint32_t QW_JWT_TTL_SEC = 3000;
static const int32_t QW_JWT_IAT_SKEW_SEC = 30;  // iat = now - 30

static const uint8_t QW_ED25519_PRIVATE_KEY[32] = {
  0x43,0x17,0x3f,0x35,0x4e,0x7c,0x87,0x66,0xc9,0xd2,0xc3,0x7e,0x63,0xf6,0x36,0xfc,
  0xb9,0x5b,0xd3,0xda,0xc4,0x6d,0x62,0xbd,0x09,0x30,0x8d,0xd1,0x31,0xab,0x22,0x7b
};
static const uint8_t QW_ED25519_PUBLIC_KEY[32] = {
  0x9a,0xb5,0x4b,0x8a,0x6b,0xdc,0x7f,0xc7,0x93,0x3a,0x05,0x54,0xaf,0x7d,0x11,0x36,
  0x58,0xd2,0x8d,0x4b,0x80,0xdc,0xff,0x3d,0x81,0x8c,0x20,0x96,0xb8,0x76,0xee,0x2a
};

static String urlEncode(const String& input) {
  const char* hex = "0123456789ABCDEF";
  String out;
  out.reserve(input.length() * 3);

  for (size_t i = 0; i < input.length(); ++i) {
    uint8_t c = (uint8_t)input[i];
    bool safe =
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-' || c == '_' || c == '.' || c == '~';

    if (safe) out += (char)c;
    else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

static String weatherTextEnByCode(int code) {
  if (code == 100 || code == 150) return "Sunny";
  if (code == 101 || code == 102 || code == 103 || code == 104 || code == 151 || code == 152 || code == 153) return "Cloudy";
  if (code == 309) return "Sun Shower";
  if (code >= 300 && code <= 399) return "Rain";
  if (code >= 400 && code <= 499 || code == 901) return "Snow";
  if (code >= 500 && code <= 515) return "Fog/Haze";
  if (code == 900) return "Hot";
  return "Unknown";
}

static bool isAsciiOnly(const String& s) {
  for (size_t i = 0; i < s.length(); ++i) {
    if ((uint8_t)s[i] >= 0x80) return false;
  }
  return true;
}

static String base64UrlEncode(const uint8_t* data, size_t len) {
  size_t outMax = 4 * ((len + 2) / 3) + 4;
  std::vector<uint8_t> out(outMax);
  size_t outLen = 0;
  int rc = mbedtls_base64_encode(out.data(), out.size(), &outLen, data, len);
  if (rc != 0 || outLen == 0) return String();

  String s;
  s.reserve(outLen + 1);
  for (size_t i = 0; i < outLen; ++i) {
    char c = (char)out[i];
    if (c == '+') c = '-';
    else if (c == '/') c = '_';
    if (c != '=') s += c;
  }
  return s;
}

QWeatherClient::QWeatherClient() {}

bool QWeatherClient::begin(const String& cityName) {
  setCity(cityName);
  ready_ = false;
  lastError_ = "";
  return true;
}

void QWeatherClient::setCity(const String& cityName) {
  city_ = cityName;
  city_.trim();
  if (city_.isEmpty()) city_ = "shanghai";

  locationId_ = "";   // 必须触发重新 lookup
  ready_ = false;
}

String QWeatherClient::getCity() const { return city_; }
bool QWeatherClient::isReady() const { return ready_; }
CurrentWeather QWeatherClient::getCurrent() const { return current_; }
const ForecastDay* QWeatherClient::getForecast3() const { return forecast_; }
String QWeatherClient::getLastError() const { return lastError_; }
String QWeatherClient::getLocationId() const { return locationId_; }

bool QWeatherClient::waitForValidTime(uint32_t timeoutMs) {
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp.ntsc.ac.cn", "pool.ntp.org");
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    time_t now = time(nullptr);
    if (now > 1700000000) return true;
    delay(250);
  }
  return false;
}

bool QWeatherClient::generateJwt() {
  time_t now = time(nullptr);
  if (now <= 1700000000) {
    lastError_ = "time not ready";
    Serial.println("[JWT] 系统时间未就绪，无法签发 token");
    return false;
  }

  uint32_t iat = (uint32_t)(now - QW_JWT_IAT_SKEW_SEC);
  uint32_t exp = (uint32_t)(iat + QW_JWT_TTL_SEC);

  String header = String("{\"alg\":\"EdDSA\",\"kid\":\"") + QW_KID + "\",\"typ\":\"JWT\"}";
  String payload = String("{\"sub\":\"") + QW_SUB + "\",\"iat\":" + String(iat) +
                   ",\"exp\":" + String(exp) + "}";

  String h64 = base64UrlEncode((const uint8_t*)header.c_str(), header.length());
  String p64 = base64UrlEncode((const uint8_t*)payload.c_str(), payload.length());
  if (h64.isEmpty() || p64.isEmpty()) {
    lastError_ = "base64url encode failed";
    return false;
  }

  String signingInput = h64 + "." + p64;
  uint8_t sig[64];
  Ed25519::sign(sig,
                QW_ED25519_PRIVATE_KEY,
                QW_ED25519_PUBLIC_KEY,
                (const uint8_t*)signingInput.c_str(),
                signingInput.length());

  String s64 = base64UrlEncode(sig, sizeof(sig));
  if (s64.isEmpty()) {
    lastError_ = "signature encode failed";
    return false;
  }

  bearerToken_ = signingInput + "." + s64;
  tokenExp_ = exp;

  Serial.printf("[JWT] now=%lu, iat=%lu(now-%ld), exp=%lu(ttl=%lu)\n",
                (unsigned long)now,
                (unsigned long)iat,
                (long)QW_JWT_IAT_SKEW_SEC,
                (unsigned long)exp,
                (unsigned long)QW_JWT_TTL_SEC);
  return true;
}

bool QWeatherClient::ensureTokenValid() {
  time_t now = time(nullptr);
  if (now <= 1700000000 && !waitForValidTime()) {
    lastError_ = "NTP sync failed";
    return false;
  }

  now = time(nullptr);
  if (bearerToken_.isEmpty() || tokenExp_ <= (uint32_t)(now + 60)) {
    return generateJwt();
  }
  return true;
}

bool QWeatherClient::readHttpBodyBytes(HTTPClient& http, std::vector<uint8_t>& out) {
  WiFiClient* stream = http.getStreamPtr();
  if (!stream) return false;

  int len = http.getSize();
  out.clear();
  out.reserve(len > 0 ? len : 4096);

  uint8_t buff[256];
  unsigned long lastDataMs = millis();
  while (http.connected() && (len > 0 || len == -1)) {
    size_t sizeAvail = stream->available();
    if (sizeAvail) {
      int c = stream->readBytes(buff, (sizeAvail > sizeof(buff)) ? sizeof(buff) : sizeAvail);
      if (c > 0) {
        out.insert(out.end(), buff, buff + c);
        lastDataMs = millis();
        if (len > 0) len -= c;
      }
    } else {
      if (millis() - lastDataMs > 8000) break;
      delay(1);
    }
  }
  return !out.empty();
}

bool QWeatherClient::isGzipData(const std::vector<uint8_t>& data) {
  return data.size() >= 2 && data[0] == 0x1F && data[1] == 0x8B;
}

String QWeatherClient::bytesToString(const std::vector<uint8_t>& data) {
  String s;
  s.reserve(data.size() + 1);
  for (size_t i = 0; i < data.size(); ++i) s += (char)data[i];
  return s;
}

String QWeatherClient::decodeBodyToJsonText(HTTPClient& http, const std::vector<uint8_t>& rawBody) {
  String encoding = http.header("Content-Encoding");
  encoding.toLowerCase();

  bool gzipByHeader = encoding.indexOf("gzip") >= 0;
  bool gzipByMagic = isGzipData(rawBody);

  if (!(gzipByHeader || gzipByMagic)) return bytesToString(rawBody);

  uint8_t* outBuf = nullptr;
  uint32_t outSize = 0;
  std::vector<uint8_t> inCopy = rawBody;
  int32_t ret = ArduinoUZlib::decompress(inCopy.data(), inCopy.size(), outBuf, outSize);
  if (ret <= 0 || outBuf == nullptr || outSize == 0) {
    if (outBuf) free(outBuf);
    return String();
  }

  String json;
  json.reserve(outSize + 1);
  for (uint32_t i = 0; i < outSize; ++i) json += (char)outBuf[i];
  free(outBuf);
  return json;
}

void QWeatherClient::logAuthFailureHint(int httpCode, const String& bodyText) {
  if (httpCode != 401 && httpCode != 403) return;

  Serial.printf("[AUTH] HTTP %d 鉴权失败\n", httpCode);
  Serial.println("[AUTH] 排查项: 1) NTP时间 2) kid/sub 3) Host 4) Ed25519私钥");

  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, bodyText);
  if (!err) {
    String title = doc["error"]["title"].as<String>();
    String detail = doc["error"]["detail"].as<String>();
    if (title.length() > 0) Serial.printf("[AUTH] title : %s\n", title.c_str());
    if (detail.length() > 0) Serial.printf("[AUTH] detail: %s\n", detail.c_str());
  }
}

bool QWeatherClient::httpGetQWeather(const String& url, String& jsonOut) {
  if (!ensureTokenValid()) return false;

  WiFiClientSecure client;
  client.setInsecure();  // 先保持与已跑通调试版本一致

  HTTPClient http;
  if (!http.begin(client, url)) {
    lastError_ = "http.begin failed";
    return false;
  }

  http.setTimeout(15000);
  http.addHeader("Authorization", String("Bearer ") + bearerToken_);
  http.addHeader("Accept", "application/json");
  http.addHeader("Accept-Encoding", "gzip");
  http.collectHeaders((const char*[]) {"Content-Encoding", "Content-Type"}, 2);

  int httpCode = http.GET();
  lastHttpStatus_ = httpCode;
  if (httpCode <= 0) {
    lastError_ = String("GET failed: ") + http.errorToString(httpCode);
    http.end();
    return false;
  }

  std::vector<uint8_t> rawBody;
  if (!readHttpBodyBytes(http, rawBody)) {
    if (httpCode == 401 || httpCode == 403) {
      Serial.println("[AUTH] 响应体为空，无法解析错误详情。请优先检查时间/kid/sub/host/私钥。");
    }
    lastError_ = "empty body";
    http.end();
    return false;
  }

  String bodyText = decodeBodyToJsonText(http, rawBody);
  if (bodyText.isEmpty()) {
    lastError_ = "decode body failed";
    http.end();
    return false;
  }

  if (httpCode == 401 || httpCode == 403) {
    logAuthFailureHint(httpCode, bodyText);
  }

  jsonOut = bodyText;
  http.end();
  return true;
}

bool QWeatherClient::parseQWeatherCode(const String& json, String& qwCode) {
  DynamicJsonDocument doc(32 * 1024);
  auto err = deserializeJson(doc, json);
  if (err) {
    lastError_ = String("json parse failed: ") + err.c_str();
    return false;
  }

  qwCode = doc["code"].as<String>();
  if (qwCode.length() == 0) {
    if (lastHttpStatus_ == 401 || lastHttpStatus_ == 403) {
      lastError_ = "authentication failed";
    } else {
      lastError_ = "qweather code missing";
    }
    return false;
  }
  return true;
}

bool QWeatherClient::doCityLookup() {
  String url = String("https://") + QW_API_HOST + "/geo/v2/city/lookup?location=" + urlEncode(city_);
  String json;
  if (!httpGetQWeather(url, json)) return false;

  String qwCode;
  if (!parseQWeatherCode(json, qwCode) || qwCode != "200") {
    if (qwCode.length() > 0) lastError_ = String("city lookup code: ") + qwCode;
    return false;
  }

  DynamicJsonDocument doc(32 * 1024);
  auto err = deserializeJson(doc, json);
  if (err) {
    lastError_ = String("city lookup parse failed: ") + err.c_str();
    return false;
  }

  JsonArray arr = doc["location"].as<JsonArray>();
  if (arr.isNull() || arr.size() == 0) {
    lastError_ = "city lookup empty";
    return false;
  }

  locationId_ = arr[0]["id"].as<String>();
  return locationId_.length() > 0;
}

bool QWeatherClient::doWeatherNow(CurrentWeather& out) {
  String url = String("https://") + QW_API_HOST + "/v7/weather/now?location=" + locationId_ + "&lang=en";
  String json;
  if (!httpGetQWeather(url, json)) return false;

  DynamicJsonDocument doc(48 * 1024);
  auto err = deserializeJson(doc, json);
  if (err) {
    lastError_ = String("weather now parse failed: ") + err.c_str();
    return false;
  }

  String qwCode = doc["code"].as<String>();
  if (qwCode != "200") {
    lastError_ = String("weather now code: ") + qwCode;
    return false;
  }

  JsonObject now = doc["now"].as<JsonObject>();
  if (now.isNull()) {
    lastError_ = "weather now missing now object";
    return false;
  }

  out.city = city_;
  out.code = now["icon"].as<String>().toInt();
  out.text = now["text"].as<String>();
  if (out.text.length() == 0 || !isAsciiOnly(out.text)) {
    out.text = weatherTextEnByCode(out.code);
  }
  out.temp = now["temp"].as<String>().toInt();
  out.feelsLike = now["feelsLike"].as<String>().toInt();
  out.humidity = now["humidity"].as<String>().toInt();
  out.windSpeed = now["windSpeed"].as<String>().toFloat();
  return true;
}

bool QWeatherClient::doWeather3D(ForecastDay out[3]) {
  String url = String("https://") + QW_API_HOST + "/v7/weather/3d?location=" + locationId_;
  String json;
  if (!httpGetQWeather(url, json)) return false;

  DynamicJsonDocument doc(64 * 1024);
  auto err = deserializeJson(doc, json);
  if (err) {
    lastError_ = String("weather 3d parse failed: ") + err.c_str();
    return false;
  }

  String qwCode = doc["code"].as<String>();
  if (qwCode != "200") {
    lastError_ = String("weather 3d code: ") + qwCode;
    return false;
  }

  JsonArray daily = doc["daily"].as<JsonArray>();
  if (daily.isNull() || daily.size() < 3) {
    lastError_ = "weather 3d daily<3";
    return false;
  }

  for (int i = 0; i < 3; ++i) {
    JsonObject d = daily[i];
    String fxDate = d["fxDate"].as<String>();
    if (fxDate.length() >= 10) out[i].date = fxDate.substring(5, 10);
    else out[i].date = fxDate;

    out[i].code = d["iconDay"].as<String>().toInt();
    out[i].tempMin = d["tempMin"].as<String>().toInt();
    out[i].tempMax = d["tempMax"].as<String>().toInt();
  }

  return true;
}

bool QWeatherClient::update() {
  if (WiFi.status() != WL_CONNECTED) {
    lastError_ = "wifi not connected";
    return false;
  }

  if (!ensureTokenValid()) return false;

  if (locationId_.isEmpty()) {
    if (!doCityLookup()) return false;
  }

  CurrentWeather newCurrent = current_;
  ForecastDay newForecast[3] = {forecast_[0], forecast_[1], forecast_[2]};

  bool okNow = doWeatherNow(newCurrent);
  bool ok3d = doWeather3D(newForecast);

  if (!okNow || !ok3d) {
    // 失败时保留旧数据，不清空
    ready_ = (current_.text.length() > 0);
    return false;
  }

  current_ = newCurrent;
  for (int i = 0; i < 3; ++i) forecast_[i] = newForecast[i];

  ready_ = true;
  lastError_ = "";
  return true;
}
