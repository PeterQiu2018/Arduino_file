#include "WeatherForecast.h"
#include <HTTPClient.h>
#include <WiFi.h>
WeatherForecast::WeatherForecast(String apiKey, String location)
{
    this->apiKey = apiKey;
    this->location = location;
}

bool WeatherForecast::update()
{
    HTTPClient http;   //用于访问网络
    WiFiClient *stream;
    int size;
    
    http.begin("https://devapi.heweather.net/v7/weather/3d?location="+ this->location + "&key=" + this->apiKey); //获取近三天天气信息
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
    deserializeJson(doc, jsonStr);
    free(outbuf);
    // 解析Json信息
    const char *code = doc["code"];
    const char *updateTime = doc["updateTime"];
    JsonArray daily = doc["daily"];
    JsonObject daily_0 = daily[0];
    JsonObject daily_1 = daily[1];
    JsonObject daily_2 = daily[2];

    _response_code = doc["code"].as<String>();
    _last_update_str = doc["updateTime"].as<String>();
    _daily_sunrise_str[0] = daily_0["sunrise"].as<String>();
    _daily_sunrise_str[1] = daily_1["sunrise"].as<String>();
    _daily_sunrise_str[2] = daily_2["sunrise"].as<String>();
    _daily_tempMax_int[0] = daily_0["tempMax"].as<int>();
    _daily_tempMax_int[1] = daily_1["tempMax"].as<int>();
    _daily_tempMax_int[2] = daily_2["tempMax"].as<int>();
    _daily_tempMin_int[0] = daily_0["tempMin"].as<int>();
    _daily_tempMin_int[1] = daily_1["tempMin"].as<int>();
    _daily_tempMin_int[2] = daily_2["tempMin"].as<int>();
    _daily_iconDay_int[0] = daily_0["iconDay"].as<int>();
    _daily_iconDay_int[1] = daily_1["iconDay"].as<int>();
    _daily_iconDay_int[2] = daily_2["iconDay"].as<int>();
    _daily_textDay_str[0] = daily_0["textDay"].as<String>();
    _daily_textDay_str[1] = daily_1["textDay"].as<String>();
    _daily_textDay_str[2] = daily_2["textDay"].as<String>();
    _daily_windDirDay_str[0] = daily_0["windDirDay"].as<String>();
    _daily_windDirDay_str[1] = daily_1["windDirDay"].as<String>();
    _daily_windDirDay_str[2] = daily_2["windDirDay"].as<String>();
    _daily_windScaleDay_str[0] = daily_0["windScaleDay"].as<String>();
    _daily_windScaleDay_str[1] = daily_1["windScaleDay"].as<String>();
    _daily_windScaleDay_str[2] = daily_2["windScaleDay"].as<String>();
    _daily_humidity_int[0] = daily_0["humidity"].as<int>();
    _daily_humidity_int[1] = daily_1["humidity"].as<int>();
    _daily_humidity_int[2] = daily_2["humidity"].as<int>();
    _daily_precip_float[0] = daily_0["precip"].as<float>();
    _daily_precip_float[1] = daily_1["precip"].as<float>();
    _daily_precip_float[2] = daily_2["precip"].as<float>();
    _daily_uvIndex_int[0] = daily_0["uvIndex"].as<int>();
    _daily_uvIndex_int[1] = daily_1["uvIndex"].as<int>();
    _daily_uvIndex_int[2] = daily_2["uvIndex"].as<int>();
    return true;
}

// 返回API状态码
String WeatherForecast::getServerCode()
{
    return _response_code;
}

// 返回当前API最近更新时间
String WeatherForecast::getLastUpdate()
{
    return _last_update_str;
}

// 返回日出时间
String WeatherForecast::getSunRise(int index)
{
    return _daily_sunrise_str[index];
}

// 返回最高气温
int WeatherForecast::getTempMax(int index)
{
    return _daily_tempMax_int[index];
}

// 返回最低气温
int WeatherForecast::getTempMin(int index)
{
    return _daily_tempMin_int[index];
}

// 返回白天天气状况的图标代码
int WeatherForecast::getIconDay(int index)
{
    return _daily_iconDay_int[index];
}

// 返回天气状况的文字描述
String WeatherForecast::getTextDay(int index)
{
    return _daily_textDay_str[index];
}

// 返回白天风向
String WeatherForecast::getWindDirDay(int index)
{
    return _daily_windDirDay_str[index];
}

// 返回白天风力等级
String WeatherForecast::getwindScaleDay(int index)
{
    return _daily_windScaleDay_str[index];
}

// 返回当天相对湿度百分比数值
int WeatherForecast::getHumidity(int index)
{
    return _daily_humidity_int[index];
}

// 返回当天降水量，毫米
float WeatherForecast::getPrecip(int index)
{
    return _daily_precip_float[index];
}

// 返回当天紫外线强度指数
int WeatherForecast::getUvIndex(int index)
{
    return _daily_uvIndex_int[index];
}
