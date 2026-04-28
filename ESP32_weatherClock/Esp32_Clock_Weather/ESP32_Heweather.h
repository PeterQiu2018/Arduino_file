#ifndef _ESP8266_HEWEATHER_H_
#define _ESP8266_HEWEATHER_H_

#include "WeatherNow.h"
#include "WeatherForecast.h"
#endif

typedef struct HeFengCurrentData {
  String weather;
  String temp;
  String temp_min;
  String temp_max;
  String humidity;
  String windSpeed;
  String icon;
}
HeFengCurrentData;
typedef struct HeFengForeData {
  String datestr;
  String temp_min;
  String temp_max;
  String icon;
}
HeFengForeData;