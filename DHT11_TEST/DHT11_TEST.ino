#include <DHT.h>
#include <DHT_U.h>
#include <Arduino.h>

DHT dht(pin_dht, DHT11);

void setup()			//Arduino程序初始化程序放在这里，只在开机时候运行一次
{
  Serial.begin(9600);	//设置通讯的波特率为9600
}

void loop()			//Arduino程序的主程序部分，循环运行内部程序
{
  temperature = dht.readTemperature();  //获取温度
  humidity = dht.readHumidity();  //获取湿度
  // 输出温度/湿度
  Serial.print(temperature, 1);
  Serial.print("/");
  Serial.println(humidity, 1);
  delay(2000);  //延迟2秒
}
