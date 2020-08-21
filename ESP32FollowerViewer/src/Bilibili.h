#ifndef __BILIBILI_H
#define __BILIBILI_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

extern String NAME ;  //�ĳ��Լ�������
extern String UID ;  //�ĳ��Լ���UID 19577966  435162639 483818980
extern String MyUID  ;
extern String followerUrl ;   // ��˿��
extern String MyFollowerUrl ;   // ��˿��
extern String viewAndLikesUrl ; // ��������������
extern String weatherUrl ;   // ʱ������

extern long follower ;   // ��˿��
extern long view ;       // ������
extern long likes ;      // ������
extern int weatherCode;
extern int temperature;

void getViewAndLikes(String url);
void getFollower(String url);
void getWeather(String url);

#endif 

