#ifndef _WEATHERFORECAST_H_
#define _WEATHERFORECAST_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoUZlib.h>
//#define DEBUG // 调试用

class WeatherForecast {
  public:
    WeatherForecast(String apiKey, String location);
    bool update();
    String getServerCode();
    String getLastUpdate();
    String getSunRise(int index);
    int getTempMax(int index);
    int getTempMin(int index);
    int getIconDay(int index);
    String getTextDay(int index);
    String getWindDirDay(int index);
    String getwindScaleDay(int index);
    int getHumidity(int index);
    float getPrecip(int index);
    int getUvIndex(int index);

  private:
    const char* _host = "devapi.heweather.net"; // 服务器地址
    const int httpsPort = 443;

    String apiKey;  // 私钥
    String location; // 位置
    StaticJsonDocument<2048> doc;   //Json容器
    JsonObject today;     //可根据返回的json信息自行定义其他天气数据

    void _parseNowJson(String payload); // 解析json信息

    String _response_code = "no_init";           // API状态码
    String _last_update_str = "no_init";         // API最近更新时间
    String _daily_sunrise_str[3] = {"no_init", "no_init", "no_init"}; // 日出时间
    int _daily_tempMax_int[3] = {999, 999, 999}; // 最高气温
    int _daily_tempMin_int[3] = {999, 999, 999}; // 最低气温
    int _daily_iconDay_int[3] = {999, 999, 999}; // 白天天气状况的图标代码
    String _daily_textDay_str[3] = {"no_init", "no_init", "no_init"};      // 天气状况的文字描述
    String _daily_windDirDay_str[3] = {"no_init", "no_init", "no_init"};   // 白天风向
    String _daily_windScaleDay_str[3] = {"no_init", "no_init", "no_init"}; // 白天风力等级
    int _daily_humidity_int[3] = {999, 999, 999};   // 相对湿度百分比数值
    float _daily_precip_float[3] = {999, 999, 999}; // 降水量，毫米
    int _daily_uvIndex_int[3] = {999, 999, 999};    // 紫外线强度指数

};
#endif
