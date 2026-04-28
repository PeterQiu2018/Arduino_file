#ifndef _WEATHERNOW_H_
#define _WEATHERNOW_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoUZlib.h>
#define DEBUG // 调试用

class WeatherNow {
  public:
    WeatherNow(String apiKey, String location);
    bool update();
    String getServerCode();
    String getLastUpdate();
    int getTemp();
    int getFeelLike();
    int getIcon();
    String getWeatherText();
    String getWindDir();
    int getWindScale();
    float getHumidity();
    float getPrecip();

  private:
    const char* _host = "devapi.heweather.net"; // 服务器地址
    const int httpsPort = 443;

    String apiKey;  // 私钥
    String location; // 位置
    StaticJsonDocument<2048> doc;   //Json容器
    JsonObject today;     //可根据返回的json信息自行定义其他天气数据

    String _response_code =  "no_init";  // API状态码
    String _last_update_str = "no_init"; // 当前API最近更新时间
    int _now_temp_int = 999;             // 实况温度
    int _now_feelsLike_int = 999;        // 实况体感温度
    int _now_icon_int = 999;             // 当前天气状况和图标的代码
    String _now_text_str = "no_init";    // 实况天气状况的文字描述
    String _now_windDir_str = "no_init"; // 实况风向
    int _now_windScale_int = 999;        // 实况风力等级
    float _now_humidity_float = 999;     // 实况相对湿度百分比数值
    float _now_precip_float = 999;       // 实况降水量,毫米
};

#endif
