#ifndef __BILIBILI_H
#define __BILIBILI_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

extern String NAME ;  //改成自己的名字
extern String UID ;  //改成自己的UID 19577966  435162639 483818980
extern String MyUID  ;
extern String followerUrl ;   // 粉丝数
extern String MyFollowerUrl ;   // 粉丝数
extern String viewAndLikesUrl ; // 播放数、点赞数
extern String weatherUrl ;   // 时间日期

extern long follower ;   // 粉丝数
extern long view ;       // 播放数
extern long likes ;      // 获赞数
extern int weatherCode;
extern int temperature;

void getViewAndLikes(String url);
void getFollower(String url);
void getWeather(String url);

#endif 

