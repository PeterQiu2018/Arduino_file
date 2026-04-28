#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoUZlib.h>   // gzip 解压库
#include <vector>
#include <time.h>
#include "mbedtls/base64.h"
#include <Ed25519.h>

// ===================== 可改参数 =====================
const char* WIFI_SSID = "Dy_test";
const char* WIFI_PASS = "dy123456";
const char* QW_API_HOST = "m36e48jcjm.re.qweatherapi.com";         // 例如: m36e48jcjm.re.qweatherapi.com
const char* CITY_NAME = "杭州";

// QWeather JWT 参数（设备端本地签发）
const char* QW_KID = "TFWG39JNJ8";
const char* QW_SUB = "2D88DP9BBU";
const uint32_t QW_JWT_TTL_SEC = 3000;
const int32_t QW_JWT_IAT_SKEW_SEC = 30;   // iat = now - 30

// Ed25519 Raw 私钥/公钥（32字节）
static const uint8_t QW_ED25519_PRIVATE_KEY[32] = {
  0x43,0x17,0x3f,0x35,0x4e,0x7c,0x87,0x66,0xc9,0xd2,0xc3,0x7e,0x63,0xf6,0x36,0xfc,
  0xb9,0x5b,0xd3,0xda,0xc4,0x6d,0x62,0xbd,0x09,0x30,0x8d,0xd1,0x31,0xab,0x22,0x7b
};
static const uint8_t QW_ED25519_PUBLIC_KEY[32] = {
  0x9a,0xb5,0x4b,0x8a,0x6b,0xdc,0x7f,0xc7,0x93,0x3a,0x05,0x54,0xaf,0x7d,0x11,0x36,
  0x58,0xd2,0x8d,0x4b,0x80,0xdc,0xff,0x3d,0x81,0x8c,0x20,0x96,0xb8,0x76,0xee,0x2a
};
// ====================================================

String g_locationId;
int g_lastHttpStatus = 0;
String g_qwBearerToken;

void logAuthFailureHint(int httpCode, const String& bodyText);

String base64UrlEncode(const uint8_t* data, size_t len) {
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

bool waitForValidTime(uint32_t timeoutMs = 15000) {
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp.ntsc.ac.cn", "pool.ntp.org");
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    time_t now = time(nullptr);
    if (now > 1700000000) return true;
    delay(250);
  }
  return false;
}

bool generateQWeatherJwt(String& tokenOut) {
  time_t now = time(nullptr);
  if (now <= 1700000000) {
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
  if (h64.isEmpty() || p64.isEmpty()) return false;

  String signingInput = h64 + "." + p64;

  uint8_t sig[64];
  Ed25519::sign(sig,
                QW_ED25519_PRIVATE_KEY,
                QW_ED25519_PUBLIC_KEY,
                (const uint8_t*)signingInput.c_str(),
                signingInput.length());

  String s64 = base64UrlEncode(sig, sizeof(sig));
  if (s64.isEmpty()) return false;

  tokenOut = signingInput + "." + s64;
  Serial.printf("[JWT] now=%lu, iat=%lu(now-%ld), exp=%lu(ttl=%lu)\n",
                (unsigned long)now,
                (unsigned long)iat,
                (long)QW_JWT_IAT_SKEW_SEC,
                (unsigned long)exp,
                (unsigned long)QW_JWT_TTL_SEC);
  return true;
}

String b64UrlToB64(String in) {
  in.replace('-', '+');
  in.replace('_', '/');
  int pad = (4 - (in.length() % 4)) % 4;
  while (pad--) in += '=';
  return in;
}

bool decodeJwtExp(const String& jwt, long& expOut) {
  int p1 = jwt.indexOf('.');
  int p2 = jwt.indexOf('.', p1 + 1);
  if (p1 <= 0 || p2 <= p1 + 1) return false;

  String payloadB64 = jwt.substring(p1 + 1, p2);
  payloadB64 = b64UrlToB64(payloadB64);

  size_t outLen = 3 * (payloadB64.length() / 4) + 4;
  std::vector<uint8_t> out(outLen);
  size_t realLen = 0;
  int rc = mbedtls_base64_decode(out.data(), outLen, &realLen,
                                 (const unsigned char*)payloadB64.c_str(), payloadB64.length());
  if (rc != 0 || realLen == 0) return false;

  String payload;
  payload.reserve(realLen + 1);
  for (int i = 0; i < realLen; ++i) payload += (char)out[i];

  DynamicJsonDocument doc(1024);
  auto err = deserializeJson(doc, payload);
  if (err || !doc["exp"].is<long>()) return false;

  expOut = doc["exp"].as<long>();
  return true;
}

String urlEncode(const String& input) {
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

    if (safe) {
      out += (char)c;
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

// 读取 HTTP 响应体（二进制）
bool readHttpBodyBytes(HTTPClient& http, std::vector<uint8_t>& out) {
  WiFiClient* stream = http.getStreamPtr();
  if (!stream) return false;

  int len = http.getSize();
  out.clear();

  if (len > 0) {
    out.reserve(len);
  } else {
    out.reserve(4096);
  }

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
      // 避免死等
      if (millis() - lastDataMs > 8000) break;
      delay(1);
    }
  }
  return !out.empty();
}

bool isGzipData(const std::vector<uint8_t>& data) {
  return data.size() >= 2 && data[0] == 0x1F && data[1] == 0x8B;
}

String bytesToString(const std::vector<uint8_t>& data) {
  if (data.empty()) return String();
  String s;
  s.reserve(data.size() + 1);
  for (size_t i = 0; i < data.size(); ++i) s += (char)data[i];
  return s;
}

String decodeBodyToJsonText(HTTPClient& http, const std::vector<uint8_t>& rawBody) {
  String encoding = http.header("Content-Encoding");
  encoding.toLowerCase();

  bool gzipByHeader = encoding.indexOf("gzip") >= 0;
  bool gzipByMagic = isGzipData(rawBody);

  Serial.printf("[HTTP] Content-Encoding: %s\n", encoding.c_str());
  Serial.printf("[HTTP] Raw bytes: %u\n", (unsigned)rawBody.size());

  if (!(gzipByHeader || gzipByMagic)) {
    Serial.println("[GZIP] Not gzip, parse as plain JSON.");
    return bytesToString(rawBody);
  }

  Serial.println("[GZIP] gzip detected, start decompress...");

  uint8_t* outBuf = nullptr;
  uint32_t outSize = 0;

  // ArduinoUZlib::decompress 需要可写入的 uint8_t*
  std::vector<uint8_t> inCopy = rawBody;
  int32_t ret = ArduinoUZlib::decompress(inCopy.data(), inCopy.size(), outBuf, outSize);

  if (ret <= 0 || outBuf == nullptr || outSize == 0) {
    Serial.printf("[GZIP] Decompress failed, ret=%d\n", (int)ret);
    if (outBuf) free(outBuf);
    return String();
  }

  String json;
  json.reserve(outSize + 1);
  for (uint32_t i = 0; i < outSize; ++i) json += (char)outBuf[i];
  free(outBuf);

  Serial.printf("[GZIP] Decompress OK, out bytes=%u\n", (unsigned)outSize);
  return json;
}

bool httpGetQWeather(const String& url, String& jsonOut) {
  WiFiClientSecure client;
  client.setInsecure(); // 调试脚本：忽略证书校验

  HTTPClient http;
  Serial.printf("\n[REQ] GET %s\n", url.c_str());

  if (!http.begin(client, url)) {
    Serial.println("[ERR] http.begin failed");
    return false;
  }

  http.setTimeout(15000);
  String authHeader = String("Bearer ") + g_qwBearerToken;
  http.addHeader("Authorization", authHeader);
  http.addHeader("Accept", "application/json");
  http.addHeader("Accept-Encoding", "gzip"); // 明确要求 gzip
  http.collectHeaders((const char*[]) {"Content-Encoding", "Content-Type"}, 2);

  int httpCode = http.GET();
  g_lastHttpStatus = httpCode;
  Serial.printf("[HTTP] Status Code: %d\n", httpCode);

  if (httpCode <= 0) {
    Serial.printf("[ERR] GET failed: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }

  std::vector<uint8_t> rawBody;
  if (!readHttpBodyBytes(http, rawBody)) {
    Serial.println("[ERR] Empty body or read failed");
    if (httpCode == 401 || httpCode == 403) {
      Serial.println("[AUTH] 响应体为空，无法解析错误详情。请优先检查时间/kid/sub/host/私钥。");
    }
    http.end();
    return false;
  }

  String bodyText = decodeBodyToJsonText(http, rawBody);

  if (bodyText.isEmpty()) {
    Serial.println("[ERR] Body decode failed");
    // 打印原始body前200字节（十六进制）
    Serial.print("[RAW HEX HEAD] ");
    for (size_t i = 0; i < rawBody.size() && i < 200; ++i) {
      Serial.printf("%02X", rawBody[i]);
    }
    Serial.println();
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

void logAuthFailureHint(int httpCode, const String& bodyText) {
  if (httpCode != 401 && httpCode != 403) return;

  Serial.printf("[AUTH] HTTP %d 鉴权失败\n", httpCode);
  Serial.println("[AUTH] 排查项: 1) NTP时间 2) kid/sub 3) Host 4) Ed25519私钥");

  DynamicJsonDocument doc(4096);
  auto err = deserializeJson(doc, bodyText);
  if (!err) {
    String detail = doc["error"]["detail"].as<String>();
    String title = doc["error"]["title"].as<String>();
    if (title.length() > 0) Serial.printf("[AUTH] title : %s\n", title.c_str());
    if (detail.length() > 0) Serial.printf("[AUTH] detail: %s\n", detail.c_str());
  }
}

bool parseQWeatherCode(const String& json, String& qwCode) {
  DynamicJsonDocument doc(32 * 1024);
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[JSON] Parse failed: %s\n", err.c_str());
    return false;
  }

  qwCode = doc["code"].as<String>();
  Serial.printf("[QWeather] code = %s\n", qwCode.c_str());
  return true;
}

bool doCityLookup(const String& cityName, String& locationIdOut) {
  String cityEncoded = urlEncode(cityName);
  String url = String("https://") + QW_API_HOST + "/geo/v2/city/lookup?location=" + cityEncoded;
  Serial.printf("[CityLookup] city encoded: %s\n", cityEncoded.c_str());

  String json;
  if (!httpGetQWeather(url, json)) {
    Serial.println("[CityLookup] HTTP failed");
    return false;
  }

  String qwCode;
  if (!parseQWeatherCode(json, qwCode)) {
    Serial.println("[CityLookup] JSON parse failed");
    if (g_lastHttpStatus == 401) {
      Serial.println("[HINT] HTTP 401：Bearer Token 过期/无效，或 Host 与签发参数不一致。");
      Serial.println("[HINT] 请检查设备时间(NTP)与 QW_KID/QW_SUB/Host 是否一致。 ");
    }
    Serial.println("[CityLookup] Body head:");
    Serial.println(json.substring(0, 400));
    return false;
  }

  if (qwCode != "200") {
    Serial.printf("[CityLookup] QWeather code not 200: %s\n", qwCode.c_str());
    Serial.println("[CityLookup] Body head:");
    Serial.println(json.substring(0, 500));
    return false;
  }

  DynamicJsonDocument doc(32 * 1024);
  auto err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[CityLookup] JSON parse failed: %s\n", err.c_str());
    return false;
  }

  JsonArray arr = doc["location"].as<JsonArray>();
  if (arr.isNull() || arr.size() == 0) {
    Serial.println("[CityLookup] location array empty");
    Serial.println(json.substring(0, 500));
    return false;
  }

  JsonObject loc = arr[0];
  locationIdOut = loc["id"].as<String>();
  String name = loc["name"].as<String>();
  String adm1 = loc["adm1"].as<String>();
  String adm2 = loc["adm2"].as<String>();

  Serial.println("[CityLookup] SUCCESS");
  Serial.printf("  location.id : %s\n", locationIdOut.c_str());
  Serial.printf("  name        : %s\n", name.c_str());
  Serial.printf("  adm1        : %s\n", adm1.c_str());
  Serial.printf("  adm2        : %s\n", adm2.c_str());

  return true;
}

bool doWeatherNow(const String& locationId) {
  String url = String("https://") + QW_API_HOST + "/v7/weather/now?location=" + locationId;

  String json;
  if (!httpGetQWeather(url, json)) {
    Serial.println("[WeatherNow] HTTP failed");
    return false;
  }

  DynamicJsonDocument doc(48 * 1024);
  auto err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[WeatherNow] JSON parse failed: %s\n", err.c_str());
    Serial.println(json.substring(0, 500));
    return false;
  }

  String qwCode = doc["code"].as<String>();
  Serial.printf("[WeatherNow] QWeather code: %s\n", qwCode.c_str());
  if (qwCode != "200") {
    Serial.println("[WeatherNow] API not success, body head:");
    Serial.println(json.substring(0, 500));
    return false;
  }

  JsonObject now = doc["now"].as<JsonObject>();
  if (now.isNull()) {
    Serial.println("[WeatherNow] now object missing");
    Serial.println(json.substring(0, 500));
    return false;
  }

  Serial.println("[WeatherNow] SUCCESS");
  Serial.printf("  text      : %s\n", now["text"].as<const char*>());
  Serial.printf("  icon      : %s\n", now["icon"].as<const char*>());
  Serial.printf("  temp      : %s\n", now["temp"].as<const char*>());
  Serial.printf("  windSpeed : %s\n", now["windSpeed"].as<const char*>());
  Serial.printf("  obsTime   : %s\n", now["obsTime"].as<const char*>());

  return true;
}

bool doWeather3D(const String& locationId) {
  String url = String("https://") + QW_API_HOST + "/v7/weather/3d?location=" + locationId;

  String json;
  if (!httpGetQWeather(url, json)) {
    Serial.println("[Weather3D] HTTP failed");
    return false;
  }

  DynamicJsonDocument doc(64 * 1024);
  auto err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("[Weather3D] JSON parse failed: %s\n", err.c_str());
    Serial.println(json.substring(0, 500));
    return false;
  }

  String qwCode = doc["code"].as<String>();
  Serial.printf("[Weather3D] QWeather code: %s\n", qwCode.c_str());
  if (qwCode != "200") {
    Serial.println("[Weather3D] API not success, body head:");
    Serial.println(json.substring(0, 500));
    return false;
  }

  JsonArray daily = doc["daily"].as<JsonArray>();
  if (daily.isNull() || daily.size() == 0) {
    Serial.println("[Weather3D] daily array missing/empty");
    Serial.println(json.substring(0, 500));
    return false;
  }

  Serial.println("[Weather3D] SUCCESS");
  for (size_t i = 0; i < daily.size(); ++i) {
    JsonObject d = daily[i];
    Serial.printf("  Day %u:\n", (unsigned)(i + 1));
    Serial.printf("    fxDate  : %s\n", d["fxDate"].as<const char*>());
    Serial.printf("    textDay : %s\n", d["textDay"].as<const char*>());
    Serial.printf("    iconDay : %s\n", d["iconDay"].as<const char*>());
    Serial.printf("    tempMin : %s\n", d["tempMin"].as<const char*>());
    Serial.printf("    tempMax : %s\n", d["tempMax"].as<const char*>());
  }

  return true;
}

void connectWiFi() {
  Serial.printf("[WiFi] Connecting to SSID: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print('.');
    if (millis() - start > 20000) {
      Serial.println("\n[WiFi] Connect timeout");
      return;
    }
  }

  Serial.println();
  Serial.println("[WiFi] Connected");
  Serial.print("[WiFi] IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(1200);
  Serial.println("\n===== Hefeng/QWeather API Debug Script =====");

  connectWiFi();
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[FATAL] WiFi not connected, stop.");
    return;
  }

  Serial.printf("[CFG] Host = %s\n", QW_API_HOST);
  Serial.printf("[CFG] City = %s\n", CITY_NAME);

  if (String(QW_API_HOST) == "your_api_host") {
    Serial.println("[FATAL] 请先配置 QW_API_HOST");
    return;
  }

  if (!waitForValidTime()) {
    Serial.println("[FATAL] NTP 校时失败，无法生成 JWT");
    return;
  }

  if (!generateQWeatherJwt(g_qwBearerToken)) {
    Serial.println("[FATAL] 设备端 JWT 生成失败");
    return;
  }

  long jwtExp = 0;
  if (decodeJwtExp(g_qwBearerToken, jwtExp)) {
    long nowUtc = (long)time(nullptr);
    Serial.printf("[JWT] exp(unix) = %ld\n", jwtExp);
    Serial.printf("[JWT] now(unix) = %ld, remain = %ld sec\n", nowUtc, jwtExp - nowUtc);
  }

  // Step1: 城市查询
  if (!doCityLookup(CITY_NAME, g_locationId)) {
    Serial.println("[FATAL] City lookup failed.");
    return;
  }

  // Step2: 当前天气
  if (!doWeatherNow(g_locationId)) {
    Serial.println("[FATAL] Weather now failed.");
    return;
  }

  // Step3: 三日预报
  if (!doWeather3D(g_locationId)) {
    Serial.println("[FATAL] Weather 3d failed.");
    return;
  }

  Serial.println("\n[ALL DONE] API link debug completed.");
}

void loop() {
  // 调试脚本：只跑一次
  delay(1000);
}
