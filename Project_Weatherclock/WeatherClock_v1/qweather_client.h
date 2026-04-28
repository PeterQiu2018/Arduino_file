#pragma once

#include <Arduino.h>
#include <vector>

struct CurrentWeather {
  String city;
  String text;
  int code = 999;
  int temp = 0;
  int feelsLike = 0;
  int humidity = 0;
  float windSpeed = 0;
};

struct ForecastDay {
  String date;  // MM-DD
  int code = 999;
  int tempMin = 0;
  int tempMax = 0;
};

class QWeatherClient {
 public:
  QWeatherClient();

  bool begin(const String& cityName);
  void setCity(const String& cityName);
  String getCity() const;

  bool update();
  bool isReady() const;

  CurrentWeather getCurrent() const;
  const ForecastDay* getForecast3() const;

  String getLastError() const;
  String getLocationId() const;

 private:
  bool ensureTokenValid();
  bool waitForValidTime(uint32_t timeoutMs = 15000);
  bool generateJwt();

  bool doCityLookup();
  bool doWeatherNow(CurrentWeather& out);
  bool doWeather3D(ForecastDay out[3]);

  bool httpGetQWeather(const String& url, String& jsonOut);
  bool parseQWeatherCode(const String& json, String& qwCode);
  String decodeBodyToJsonText(class HTTPClient& http, const std::vector<uint8_t>& rawBody);
  bool readHttpBodyBytes(class HTTPClient& http, std::vector<uint8_t>& out);
  bool isGzipData(const std::vector<uint8_t>& data);
  String bytesToString(const std::vector<uint8_t>& data);
  void logAuthFailureHint(int httpCode, const String& bodyText);

 private:
  String city_;
  String locationId_;
  String bearerToken_;
  uint32_t tokenExp_ = 0;

  CurrentWeather current_;
  ForecastDay forecast_[3];

  bool ready_ = false;
  int lastHttpStatus_ = 0;
  String lastError_;
};
