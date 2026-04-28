#include "weather.h"
#include <HTTPClient.h>
#include <WiFi.h>
Weather::Weather(String apiKey, String location)
{
    this->apiKey = apiKey;
    this->location = location;
}

bool Weather::update()
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
    deserializeJson(doc, outbuf);
    today = doc["now"].as<JsonObject>();
    return true;
}

//获取天气数据中的当天最高气温数据
int Weather::getTemp(bool is_today)
{
  return today["temp"].as<int>();
}


//获取天气数据中的天气标识代码
int Weather::getWeather(bool is_today, bool is_day)
{
  return today["icon"].as<int>();
}

