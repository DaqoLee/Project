#include "Bilibili.h"
#include <ArduinoJson.h>
#include "GUI_Paint.h"
#include "DEV_Config.h"
#include <Wire.h>

String NAME = "DaqoLee";  //改成自己的名字
String UID  = "19577966";  //改成自己的UID 19577966  435162639 483818980
String MyUID = "483818980";
String followerUrl = "http://api.bilibili.com/x/relation/stat?vmid=" + UID;   // 粉丝数
String MyFollowerUrl = "http://api.bilibili.com/x/relation/stat?vmid=" + MyUID;   // 粉丝数
String viewAndLikesUrl = "http://api.bilibili.com/x/space/upstat?mid=" + UID; // 播放数、点赞数
String weatherUrl = "http://api.seniverse.com/v3/weather/now.json?key=Sq4f2lh5b3lLPTCWr&location=guangzhou&language=zh-Hans&unit=c";   // 时间日期



long follower = 0;   // 粉丝数
long view = 0;       // 播放数
long likes = 0;      // 获赞数
int weatherCode=0;
int temperature=0;

DynamicJsonBuffer jsonBuffer(256); // ArduinoJson V5
// 获取 B 站播放数与获赞数
void getViewAndLikes(String url)
{
  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  Serial.printf("[HTTP] GET... code: %d\n", httpCode);

  if (httpCode == 200)
  {
    Serial.println("Get OK");
    String resBuff = http.getString();
    JsonObject &root = jsonBuffer.parseObject(resBuff);
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      return;
    }
    JsonObject& results_0 = root["results"][0];
    likes = root["data"]["likes"];
    view = root["data"]["archive"]["view"];
    Serial.print("Likes: ");
    Serial.println(likes);
    Serial.print("View: ");
    Serial.println(view);
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %d\n", httpCode);
  }

  http.end();
}

// 获取 B 站粉丝数
void getFollower(String url)
{
  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  Serial.printf("[HTTP] GET... code: %d\n", httpCode);

  if (httpCode == 200)
  {
    Serial.println("Get OK");
    String resBuff = http.getString();

 //   Serial.println(resBuff);
    // ---------- ArduinoJson V5 ----------
    JsonObject &root = jsonBuffer.parseObject(resBuff);
    delay(2);
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      return;
    }
    follower = root["data"]["follower"];
    Serial.print("Fans: ");
    Serial.println(follower);
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %d\n", httpCode);
  }

  http.end();
}


void getWeather(String url)
{
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.printf("[HTTP] GET... code: %d\n", httpCode);
  if (httpCode == 200)
  {
    Serial.println("Get OK");
    String resBuff = http.getString();
  //   Serial.println(resBuff);
    // ---------- ArduinoJson V5 ----------
    JsonObject &root = jsonBuffer.parseObject(resBuff);
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      return;
    }
   // JsonObject& results_0 = root["results"][0];
    weatherCode = root["results"][0]["now"]["code"];
    temperature = root["results"][0]["now"]["temperature"];
     Serial.println(temperature);
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %d\n", httpCode);
  }

  http.end();
}
