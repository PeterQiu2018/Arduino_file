#include "HardwareSerial.h"
#include "WeatherNow.h"
#include <HTTPClient.h>
#include <WiFi.h>
WeatherNow::WeatherNow(String apiKey, String location)
{
    this->apiKey = apiKey;
    this->location = location;
}

bool WeatherNow::update()
{
    HTTPClient http;   //用于访问网络
    WiFiClient *stream;
    int size;
    
    http.begin("https://devapi.heweather.net/v7/weather/now?location="+ this->location + "&key=" + this->apiKey); //获取近三天天气信息
    int httpcode = http.GET();   //发送GET请求
    if(httpcode > 0)
    {
        if(httpcode == HTTP_CODE_OK)
        {
            stream = http.getStreamPtr();   //读取服务器返回的数据
            size = http.getSize();
        }
    }
    else
    {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpcode).c_str());
        
    }
    http.end();   //结束当前连接
    
    uint8_t inbuff[size];
    stream->readBytes(inbuff, size);
    uint8_t *outbuf = NULL;
    uint32_t out_size = 0;
    int result = ArduinoUZlib::decompress(inbuff,size, outbuf, out_size);
    String jsonStr = String((char*)outbuf).substring(0, out_size);
    Serial.println(jsonStr);
    deserializeJson(doc, jsonStr);
    free(outbuf);
    // 解析Json信息
    _response_code = doc["code"].as<String>();          // API状态码
    _last_update_str = doc["updateTime"].as<String>();  // 当前API最近更新时间
    JsonObject now = doc["now"];
    _now_temp_int = now["temp"].as<int>();              // 实况温度
    _now_feelsLike_int = now["feelsLike"].as<int>();    // 实况体感温度
    _now_icon_int = now["icon"].as<int>();              // 当前天气状况和图标的代码
    _now_text_str = now["text"].as<String>();           // 实况天气状况的文字描述
    _now_windDir_str = now["windDir"].as<String>();     // 实况风向
    _now_windScale_int = now["windScale"].as<int>();    // 实况风力等级
    _now_humidity_float = now["humidity"].as<int>();  // 实况相对湿度百分比数值
    _now_precip_float = now["precip"].as<float>();      // 实况降水量,毫米
    Serial.println(_now_text_str);
    return true;
}

// API状态码
String WeatherNow::getServerCode() {
  return _response_code;
}

// 当前API最近更新时间
String WeatherNow::getLastUpdate() {
  return _last_update_str;
}

// 实况温度
int WeatherNow::getTemp() {
  return _now_temp_int;
}

// 实况体感温度
int WeatherNow::getFeelLike() {
  return _now_feelsLike_int;
}

// 当前天气状况和图标的代码
int WeatherNow::getIcon() {
  return _now_icon_int;
}

// 实况天气状况的文字描述
String WeatherNow::getWeatherText() {
  return _now_text_str;
}

// 实况风向
String WeatherNow::getWindDir() {
  return _now_windDir_str;
}

// 实况风力等级
int WeatherNow::getWindScale() {
  return _now_windScale_int;
}

// 实况相对湿度百分比数值
float WeatherNow::getHumidity() {
  return _now_humidity_float;
}
// 实况降水量,毫米
float WeatherNow::getPrecip() {
  return _now_precip_float;
}