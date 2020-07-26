/* Includes ------------------------------------------------------------------*/
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include <stdlib.h>

#if defined(ESP32) //ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#elif defined(ESP8266) //ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#else
#error "Please check your mode setting,it must be esp8266 or esp32."
#endif

#include <ArduinoJson.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Ticker.h>

#include <Adafruit_NeoPixel.h>

#include "Arduino.h"
#include "esp32-hal.h"

#include <TimeLib.h>
#include <WiFiUdp.h> 

static const char ntpServerName[] = "time1.aliyun.com";//阿里云的时间服务器
const int timeZone = 8;     // 时区
 
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
 
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define LED1_PIN    17
#define LED2_PIN    18
#define LED3_PIN    19
#define LED4_PIN    21
#define LED5_PIN    22
#define LED6_PIN    23

#define NR_OF_LEDS   30
#define NR_OF_ALL_BITS 24*NR_OF_LEDS

rmt_data_t led_data[NR_OF_ALL_BITS];

rmt_obj_t* rmt_send_1 = NULL;
rmt_obj_t* rmt_send_2 = NULL;
rmt_obj_t* rmt_send_3 = NULL;
rmt_obj_t* rmt_send_4 = NULL;
rmt_obj_t* rmt_send_5 = NULL;
rmt_obj_t* rmt_send_6 = NULL;

// 定时器
Ticker timer;
int count = 0;
boolean flag = true;

// JSON
DynamicJsonBuffer jsonBuffer(512); // ArduinoJson V5

// WiFi 名称与密码
const char *ssid = "2506"; //这里填你家中的wifi名  碳酸镁 OnePlus 6
const char *password = "15814506697";//这里填你家中的wifi密码  cjd84428293  asdfghjkl
// B 站 API 网址: follower, view, likes
String NAME = "DaqoLee";  //改成自己的名字
String UID  = "19577966";  //改成自己的UID 19577966  435162639
String followerUrl = "http://api.bilibili.com/x/relation/stat?vmid=" + UID;   // 粉丝数
String viewAndLikesUrl = "http://api.bilibili.com/x/space/upstat?mid=" + UID; // 播放数、点赞数
String weatherUrl = "http://api.seniverse.com/v3/weather/now.json?key=Sq4f2lh5b3lLPTCWr&location=guangzhou&language=zh-Hans&unit=c";   // 时间日期


#define TEMP 7
typedef struct SnakeNode
{
  int x;
  int y;
  
  struct SnakeNode *prev,*next;

}Snake_t;

typedef struct Food
{
  int x;
  int y;
}Food_t;

Snake_t SnakeHand;
Food_t Food;

int FoodBuf[][2]=
{
  33*TEMP,4*TEMP,
  33*TEMP,8*TEMP,
  33*TEMP,12*TEMP,
  33*TEMP,16*TEMP,
  
  31*TEMP,16*TEMP,
  31*TEMP,12*TEMP,
  31*TEMP,8*TEMP,
  31*TEMP,4*TEMP,
  31*TEMP,1*TEMP,
  
  29*TEMP,1*TEMP,
  29*TEMP,4*TEMP,
  29*TEMP,8*TEMP,
  29*TEMP,12*TEMP,
  29*TEMP,16*TEMP,
  
  27*TEMP,16*TEMP,
  27*TEMP,12*TEMP,
  27*TEMP,8*TEMP,
  27*TEMP,4*TEMP,
  27*TEMP,1*TEMP,
  
  25*TEMP,1*TEMP,
  25*TEMP,4*TEMP,
  25*TEMP,8*TEMP,
  25*TEMP,12*TEMP,
  25*TEMP,16*TEMP,
  
  23*TEMP,16*TEMP,
  23*TEMP,12*TEMP,
  23*TEMP,8*TEMP,
  23*TEMP,4*TEMP,
  
  7*TEMP,3*TEMP,
  14*TEMP,11*TEMP,
  13*TEMP,12*TEMP,
               
  7*TEMP,5*TEMP,
};

unsigned char GameMap[240][240]={0};

long follower = 0;   // 粉丝数
long view = 0;       // 播放数
long likes = 0;      // 获赞数
int weatherCode=0;
int temperature=0;

void setup()
{

  Serial.begin(115200);
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.
  rmt_send_1 = rmtInit(LED1_PIN, true, RMT_MEM_64);
  rmt_send_2 = rmtInit(LED2_PIN, true, RMT_MEM_64);
  rmt_send_3 = rmtInit(LED3_PIN, true, RMT_MEM_64);
  rmt_send_4 = rmtInit(LED4_PIN, true, RMT_MEM_64);
  rmt_send_5 = rmtInit(LED5_PIN, true, RMT_MEM_64);
  rmt_send_6 = rmtInit(LED6_PIN, true, RMT_MEM_64);

  rmtSetTick(rmt_send_1, 100);
  rmtSetTick(rmt_send_2, 100);
  rmtSetTick(rmt_send_3, 100);
  rmtSetTick(rmt_send_4, 100);
  rmtSetTick(rmt_send_5, 100);
  rmtSetTick(rmt_send_6, 100);
  // WiFi 连接
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  #if defined(ESP32)         // 当前目标板子是 ESP32
    Serial.println(Udp.remotePort());
  #elif defined(ESP8266)     // 当前目标板子是 ESP8266
    Serial.println(Udp.localPort());
  #endif
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  timer.attach(30, timerCallback); // 每隔1min
  GAME_SnakeInit(&SnakeHand,GameMap);

  // 第一次调用获取数据函数，方便开机即可显示
  getFollower(followerUrl);
  getWeather(weatherUrl);
  //墨水屏初始化
  DEV_Module_Init();
  EPD_2IN13_V2_Init(EPD_2IN13_V2_FULL);
  EPD_2IN13_V2_Clear();
  DEV_Delay_ms(500);
}

void loop()
{
    //Create a new image cache
  int numb=0;
  int weaTemp=1;

  UBYTE *BlackImage;
  /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
  UWORD Imagesize = ((EPD_2IN13_V2_WIDTH % 8 == 0) ? (EPD_2IN13_V2_WIDTH / 8 ) : (EPD_2IN13_V2_WIDTH / 8 + 1)) * EPD_2IN13_V2_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    printf("Failed to apply for black memory...\r\n");
    while (1);
  }
  printf("Paint_NewImage\r\n");
  Paint_NewImage(BlackImage, EPD_2IN13_V2_WIDTH, EPD_2IN13_V2_HEIGHT, 180, WHITE);
  Paint_SelectImage(BlackImage);
  Paint_SetMirroring(MIRROR_HORIZONTAL); //
  Paint_Clear(WHITE);
#if   0
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_DrawBitMap(gImage_66); //gImage_66  
  EPD_2IN13_V2_Display(BlackImage);
#endif
//  DEV_Delay_ms(1000);
  EPD_2IN13_V2_Init(EPD_2IN13_V2_FULL);
  EPD_2IN13_V2_DisplayPartBaseImage(BlackImage);
  EPD_2IN13_V2_Init(EPD_2IN13_V2_PART);
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  GlowTubeTest(weaTemp);
  while (1)
  {
     while(0)//(flag)
     {  
        if (count == 0)
        {
          // display data
          flag = false;
        } else if (count == 1) {
          // get follower
          Serial.println("count = 1, get follower");
          getFollower(followerUrl);
          flag = false;
        } else if (count == 2) {
          // get view and likes
          Serial.println("count = 2, get view and likes");
          getWeather(weatherUrl);
          //getViewAndLikes(dateTimeUrl);
          flag = false;
        }
        now();
        ClockDisplay();
       // FollowerDisplay(follower);
    }
    
//    WeatherDisplay(weatherCode, temperature);// weaTemp
//    WeatherDisplay(weaTemp, temperature);// weaTemp
//    weaTemp=weaTemp>30?1:weaTemp+1;

//    GlowTubeTest(weaTemp);
//    Paint_DrawBitMap_1(0,40,40,120,gImage_bili);  
//    Paint_ClearWindows(10, 220, 10 + Font20.Width * 7, 220 + Font20.Height, WHITE);
//    Paint_DrawNum(10, 220, follower, &Font20, BLACK, WHITE);
    GAME_SnakeTest();
    EPD_2IN13_V2_DisplayPart(BlackImage);
    delay(100);
 }
//  EPD_2IN13_V2_Init(EPD_2IN13_V2_FULL);
//  EPD_2IN13_V2_Clear();
//  DEV_Delay_ms(2000);//Analog clock 1s
//  printf("Goto Sleep...\r\n");
//  EPD_2IN13_V2_Sleep();
//  free(BlackImage);
//  BlackImage = NULL;
//  DEV_Delay_ms(1000);//Analog clock 1s 
}
// 定时器回调函数
void timerCallback()
{
  count++;
  if (count == 3)
  {
    count = 0;
  }
  flag = true;
   
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

void getFollowerTest(String url)
{
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  
  http.end();
}


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

void GlowTubeTest(int testNum)
{

#if  0
     for(int temp=0;temp<10;temp++)
     {
       colWrite(1, temp ,255 ,0 ,0);
       delay(500);
     }
    for(int i=2;i<6;i++)
    {    
      switch(i)
      {
        case 1: 
           for(int j=0;j<10;j++)
           {
             colWrite(1, j ,255 ,0 ,0);
             delay(100);
           }
           break;
        case 2:
           for(int j=0;j<10;j++)
           {
             colWrite(1, j ,255 ,255 ,0);
             delay(100);
           }
           break;
        case 3:
           for(int j=0;j<10;j++)
           {
             colWrite(1, j ,255 ,0 ,255);
             delay(100);
           } 
            break;
        case 4:
           for(int j=0;j<10;j++)
           {
             colWrite(1, j ,0 ,255 ,0);
             delay(100);
           }
            break;
        case 5:
           for(int j=0;j<10;j++)
           {
             colWrite(1, j ,0 ,255 ,255);
             delay(100);
           }
           break;
        case 6:
           for(int j=0;j<10;j++)
           {
             colWrite(1, j ,0 ,0 ,255);
             delay(100);
           }                  
          break;
        default:
        break;
      }   
    }

   colWrite(1, 31 ,0 ,0 ,255);
//     for(int temp=0;temp<10;temp++)
//     {
//       colWrite(1, temp ,255 ,0 ,0);
//       delay(500);
//     }
     delay(2000);
     for(int temp=21;temp<=30;temp++)
     {
       colWrite(1, temp ,255 ,0 ,0);
       delay(500);
     }

      delay(8000);
#endif

#if  1 //杀马特爆闪
    for(int i=1;i<=4;i++)
    {
      for(int j=0;j<11;j++)
      {
        colWrite(1,  j,255 ,0 ,0);
        delay(2);
        colWrite(2,  j,255 ,255 ,0);
        delay(2);
        colWrite(3,  j,0 ,255 ,0);
        delay(2);
        colWrite(4,  j,0 ,255 ,255);
        delay(2);
        colWrite(5,  j,0 ,0 ,255);
        delay(2);
        colWrite(6,  j,255 ,0 ,255);
        delay(2); 
        delay(100);
      }  
    } 


#if  1 //呼吸效果
    for(int i=1, n=7;i<=4;i++)
    {
      if(i%2)
      {
        for(int j=0;j<=255;j+=3)
        {
          colWrite(1,  n,j ,0 ,0);
          delay(2);
          colWrite(2,  n,j ,j ,0);
          delay(2);
          colWrite(3,  n,0 ,j ,0);
          delay(2);
          colWrite(4,  n,0 ,j ,j);
          delay(2);
          colWrite(5,  n,0 ,0 ,j);
          delay(2);
          colWrite(6,  n,j ,0 ,j);
          delay(2); 
          delay(10);
        }   
      }
      else
      {
        for(int j=255;j>=0;j-=3)
        {
          colWrite(1,  n,j ,0 ,0);
          delay(2);
          colWrite(2,  n,j ,j ,0);
          delay(2);
          colWrite(3,  n,0 ,j ,0);
          delay(2);
          colWrite(4,  n,0 ,j ,j);
          delay(2);
          colWrite(5,  n,0 ,0 ,j);
          delay(2);
          colWrite(6,  n,j ,0 ,j);
          delay(2); 
          delay(10);
        }          
      }
    } 

#endif


#if  1 //轮流点亮
    for(int i=6;i>0;i--)
    {
      for(int j=0;j<10;j++)
      {
//        colWrite(i,  j,255 ,0 ,0);
//        delay(50); 

        switch(i)
        {
          case 1:
             colWrite(1,  j,255 ,0 ,0);   
           delay(2); 
          case 2:
             colWrite(2,  j,255 ,0 ,0);   
           delay(2); 
          case 3:
             colWrite(3,  j,255 ,0 ,0);   
           delay(2); 
          case 4:
             colWrite(4,  j,255 ,0 ,0);   
           delay(2); 
          case 5:
             colWrite(5,  j,255 ,0 ,0);     
            delay(2); 
          case 6:
             colWrite(6,  j,255 ,0 ,0);                    
            break;
          default:
          break;
        } 

        delay(50);
      }  
    } 
  //逐个淡化熄灭
    for(int n=4,i=1;i<n;i++)
    {
      if(i%2)
      {
        if(i!=n-1)
        {
          for(int j=255;j>=0;j-=3)
          {
            colWrite(1,  9,j ,0 ,0);
            delay(2);
            colWrite(2,  9,j ,0 ,0);
            delay(2);
            colWrite(3,  9,j ,0 ,0);
            delay(2);
            colWrite(4,  9,j ,0 ,0);
            delay(2);
            colWrite(5,  9,j ,0 ,0);
            delay(2);
            colWrite(6,  9,j ,0 ,0);
            delay(2); 
            delay(10);
          }  
       }
       else 
       {
          for(int i=1;i<7;i++)
          {
            for(int j=255;j>=0;j-=3)
            {
              colWrite(i,  9,j ,0 ,0);
              delay(10);
            }           
          } 
//          colWrite(6,  0,255 ,0 ,0);
//          delay(10);
       }
      }
      else 
      {
        for(int j=0;j<=255;j+=3)
        {
          colWrite(1,  9,j ,0 ,0);
          delay(2);
          colWrite(2,  9,j ,0 ,0);
          delay(2);
          colWrite(3,  9,j ,0 ,0);
          delay(2);
          colWrite(4,  9,j ,0 ,0);
          delay(2);
          colWrite(5,  9,j ,0 ,0);
          delay(2);
          colWrite(6,  9,j ,0 ,0);
          delay(2); 
          delay(10);
        }          
      }
    } 
#endif    
#endif     
#if  0    //传参控制
    colWrite(1,  testNum,255 ,0 ,0);
    delay(10);
    colWrite(2,  testNum,255 ,255 ,0);
    delay(10);
    colWrite(3,  testNum,0 ,255 ,0);
    delay(10);
    colWrite(4,  testNum,0 ,255 ,255);
    delay(10);
    colWrite(5,  testNum,0 ,0 ,255);
    delay(10);
    colWrite(6,  testNum,255 ,0 ,255);
    delay(10);
#endif 
}


void FollowerDisplay(int temp)
{
    char followerBuf[7];
    sprintf(followerBuf, "%d",temp);

//    colWrite(1,  followerBuf[1]-'0',255 ,0 ,0);
//    delay(10);
//    colWrite(2,  followerBuf[2]-'0',255 ,0 ,0);
//     delay(10);
//    colWrite(3,  followerBuf[3]-'0',255 ,0 ,0);
//     delay(10);
//    
//    colWrite(4,  followerBuf[4]-'0',255 ,0 ,0);
//     delay(10);
//    colWrite(5,  followerBuf[5]-'0',255 ,0 ,0);
//     delay(10);
//    colWrite(6,  followerBuf[6]-'0',255 ,0 ,0);
//     delay(10);

    colWrite(1,  followerBuf[1]-'0',255 ,0 ,0);
    delay(10);
    colWrite(2,  followerBuf[2]-'0',255 ,255 ,0);
    delay(10);
    colWrite(3,  followerBuf[3]-'0',0 ,255 ,0);
    delay(10);
    colWrite(4,  followerBuf[4]-'0',0 ,255 ,255);
     delay(10);
    colWrite(5,  followerBuf[5]-'0',0 ,0 ,255);
     delay(10);
    colWrite(6,  followerBuf[6]-'0',255 ,0 ,255);
     delay(10);
}

void WeatherDisplay(int weaCode, int temp)
{
    Paint_ClearWindows(0, 52, 0 + Font24.Width * 2, 52 + Font24.Height, WHITE);
    Paint_DrawNum(0, 52, temp, &Font24, BLACK, WHITE);
   switch(weaCode)
  {
    case 1:
      Paint_ClearWindows(0, 5, 0 + Font24CN.Width, 5 + Font24CN.Height, WHITE);
      Paint_DrawString_CN(0, 5,"晴", &Font24CN, BLACK, WHITE);
      Paint_DrawBitMap_1(42,170,80,80,gImage_01);                         //  Update strip to match
      break;
    case 4:
      Paint_ClearWindows(0, 5, 0 + Font24CN.Width, 5 + Font24CN.Height, WHITE);
      Paint_DrawString_CN(0, 5,"云", &Font24CN, BLACK, WHITE);
      Paint_DrawBitMap_1(42,170,80,80,gImage_04);                         //  Update strip to match
      break;
    case 9:
      Paint_ClearWindows(0, 5, 0 + Font24CN.Width, 5 + Font24CN.Height, WHITE);
      Paint_DrawString_CN(0, 5,"阴", &Font24CN, BLACK, WHITE);
      Paint_DrawBitMap_1(42,170,80,80,gImage_09);                        //  Update strip to match
      break;
    case 10:
      Paint_ClearWindows(0, 5, 0 + Font24CN.Width, 5 + Font24CN.Height, WHITE);
      Paint_DrawString_CN(0, 5,"雨", &Font24CN, BLACK, WHITE);
      Paint_DrawBitMap_1(42,170,80,80,gImage_10);                       //  Update strip to match
      break;
    case 21:
      Paint_ClearWindows(0, 5, 0 + Font24CN.Width, 5 + Font24CN.Height, WHITE);
      Paint_DrawString_CN(0, 5,"雪", &Font24CN, BLACK, WHITE);
      Paint_DrawBitMap_1(42,170,80,80,gImage_21);                       //  Update strip to match
      break;
    case 30:
      Paint_ClearWindows(0, 5, 0 + Font24CN.Width, 5 + Font24CN.Height, WHITE);
      Paint_DrawString_CN(0, 5,"雾", &Font24CN, BLACK, WHITE);
      Paint_DrawBitMap_1(42,170,80,80,gImage_30);                       //  Update strip to match
      break;
    default:
      Paint_ClearWindows(0, 5, 0 + Font24CN.Width, 5 + Font24CN.Height, WHITE);
      Paint_DrawString_CN(0, 5,"雨", &Font24CN, BLACK, WHITE);
      Paint_DrawBitMap_1(42,170,80,80,gImage_10);
    break;
  }    
}


void ClockDisplay()
{
  char timeBuf[10];
  char dateBuf[5];
  
  Paint_ClearWindows(0, 90, 0 + Font20.Width * 8+2, 90 + Font20.Height, WHITE);
  Paint_DrawNum(0, 90,year(), &Font20, BLACK, WHITE);
  sprintf(dateBuf, "%02d%02d", month(), day());
  Paint_ClearWindows(0, 110, 0 + Font20.Width * 8+2, 110 + Font20.Height, WHITE);
  Paint_DrawString_EN(0, 110, dateBuf, &Font20, BLACK, WHITE);

#if  0
  colWrite(1, hour()/10 ,255 ,0 ,0);
  delay(10);
  colWrite(2,  hour()%10,255 ,255 ,0);
  delay(10);
  colWrite(3, minute()/10,255 ,0 ,255);
  delay(10);
  colWrite(4,  minute()%10,0 ,255 ,0);
  delay(10);
  colWrite(5, second()/10,0 ,0 ,255);
  delay(10);
  colWrite(6,  second()%10,0 ,255 ,255);
  delay(10);
#endif 

  sprintf(timeBuf, "%02d:%02d:%02d", hour(), minute(), second());
  Paint_ClearWindows(5, 140, 0 + Font20.Width * 8+2, 140 + Font20.Height, WHITE);
  Paint_DrawString_EN(5, 140, timeBuf, &Font20, BLACK, WHITE);
  Paint_ClearWindows(80, 85, 80 + Font20.Width * 8+2, 85 + Font20.Height, WHITE);
  Paint_DrawString_CN(80, 85,num_week(weekday(),4), &Font24CN, BLACK, WHITE);
}

/*
@功能:判断星期并赋值
*/
char week1[10],week2[10],week3[2],week4[4];

char *num_week(uint8_t dayofweek,int Mode){
  switch(dayofweek)
  {
    case 1: 
    strcpy(week1,"Sunday");
    strcpy(week2,"周日");
    strcpy(week3,"Su");
    strcpy(week4,"日"); 
      break;
    case 2: 
    strcpy(week1,"Monday");
    strcpy(week2,"周一");
    strcpy(week3,"Mo");
    strcpy(week4,"一"); 
      break;
    case 3: 
    strcpy(week1,"Tuesday");
    strcpy(week2,"周二");
    strcpy(week3,"Tu");
    strcpy(week4,"二"); 
      break;
    case 4: 
    strcpy(week1,"Wednesday");
    strcpy(week2,"周三"); 
    strcpy(week3,"We");
    strcpy(week4,"三");  
      break;
    case 5: 
    strcpy(week1,"Thursday");
    strcpy(week2,"周四"); 
    strcpy(week3,"Th");
    strcpy(week4,"四"); 
      break;
    case 6: 
    strcpy(week1,"Friday");
    strcpy(week2,"周五");
    strcpy(week3,"Fr"); 
    strcpy(week4,"五");
      break;
    case 7: 
    strcpy(week1,"Saturday");
    strcpy(week2,"周六"); 
    strcpy(week3,"Sa");
    strcpy(week4,"六");
      break;
    default:
    strcpy(week1,"NO");
    strcpy(week2,"无");
    strcpy(week3,"NO");
    strcpy(week4,"无");
      break; 
  }
  switch(Mode)
  {
    case 1: return week1; break;
    case 2: return week2; break;
    case 3: return week3; break;
    case 4: return week4; break;
    default:
    break;
  }
}
 

 
void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}
 
/*-------- NTP code (下面不用看哦)----------*/
 
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
 
time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address
 
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}
 
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
void colWrite(int Num,int Data,int R,int G,int B)
{
    int led, col, bit;
    int i=0,j=0;

    for (led=0; led<NR_OF_LEDS; led++) 
    {
        for (j=0; j<3; j++ ) 
        {
          if(j==0)
          {
            col=G;
          }
          else if(j==1)
          {
            col=R;
          }
          else 
          {
            col=B;
          }
              
          for (bit=0; bit<8; bit++)
          {
              if ( (col & (1<<(8-bit))) && ((led == Data)||led == Data+10) ){
                  led_data[i].level0 = 1;
                  led_data[i].duration0 = 8;
                  led_data[i].level1 = 0;
                  led_data[i].duration1 = 4;
              } else {
                  led_data[i].level0 = 1;
                  led_data[i].duration0 = 4;
                  led_data[i].level1 = 0;
                  led_data[i].duration1 = 8;
              }
              i++;
          }
        }
    }

  switch(Num)
  {
    case 1:
      rmtWrite(rmt_send_1, led_data, NR_OF_ALL_BITS);                      //  Update strip to match
      break;
    case 2:
      rmtWrite(rmt_send_2, led_data, NR_OF_ALL_BITS);                         //  Update strip to match
      break;
    case 3:
      rmtWrite(rmt_send_3, led_data, NR_OF_ALL_BITS);                          //  Update strip to match
      break;
    case 4:
      rmtWrite(rmt_send_4, led_data, NR_OF_ALL_BITS);                         //  Update strip to match
      break;
    case 5:
      rmtWrite(rmt_send_5, led_data, NR_OF_ALL_BITS);                      //  Update strip to match
      break;
    case 6:
      rmtWrite(rmt_send_6, led_data, NR_OF_ALL_BITS);                    //  Update strip to match
      break;
    default:
    break;
  }
}



void GAME_SnakeTest(void)
{
  static int j=0,N=4;
   delay(10);
   if(j<156)
   {
    GAME_SnakeMove(GameMap,&SnakeHand,N);  
   } 
   j++;
   if(j==16)
    N=1;
   if(j==18)
    N=2;
   if(j==33)
    N=1;
   if(j==35)
    N=4;
   if(j==50)
    N=1;
   if(j==52)
    N=2;
   if(j==67)
    N=1;
   if(j==69)
    N=4;
   if(j==84)
    N=1;
   if(j==86)
    N=2;
   if(j==98)
    N=1;
   if(j==107)
    N=2;
   if(j==108)
    N=1;
   if(j==115)
    N=4;
   if(j==117)
    N=3;
   if(j==124)
    N=4;
   if(j==130)
    N=1;
   if(j==131)
    N=4;
   if(j==132)
    N=1;
   if(j==134)
    N=4;
   if(j==135)
    N=1;
   if(j==137)
    N=4;
   if(j==138)
    N=1;
   if(j==140)
    N=2;
   if(j==144)
    N=1;
   if(j==148)
    N=2;
   if(j==149)
    N=3;
   if(j==150)  
    N=2; 
   if(j==151)  
    N=3;   
   if(j==152)
    N=2;
   if(j==153)
    N=3;
   if(j==154)
    N=2;
   if(j==155)
    N=3;  
}
 

void GAME_SnakeFillInGameMap(unsigned char (*gamemap)[240],Snake_t *snake)
{
  Snake_t *p=snake; 
  while(p->next!=NULL){
    gamemap[p->y][p->x]=1;
    p=p->next;
  }
}

void GAME_SnakePixel(int x,int y)
{
  Paint_DrawPoint(y, x, BLACK, DOT_PIXEL_4X4, DOT_STYLE_DFT);
}



void GAME_NewFood(unsigned char (*gamemap)[240],Food_t *food,int x1,int y1)
{
  while(1)
  {    
//    food->x+=14;//random(240); 
//    food->y = 7;//random(10); 
    food->x=x1;
    food->y=y1;
    if(gamemap[food->x][food->y]==0)
    {
      gamemap[food->x][food->y]=2;
      GAME_SnakePixel(food->x,food->y);
      break;
    }
  }
}

void GAME_SnakeAddNode(Snake_t *snake,int x,int y)
{
  Snake_t *node,*p;
  p=snake;
  
  node=(Snake_t *)malloc(sizeof(Snake_t));
  
  if(node==NULL)
  {
  
  }
  node->x=x;
  node->y=y;
  node->next=NULL;

  p->prev=node;
  
  while(p->next!=NULL)
  {
    p =p->next;
  }
  node->prev=p;
  p->next=node;
  GAME_SnakeFillInGameMap(GameMap ,snake);
}

void GAME_NewSnake(Snake_t *snake)
{
  int x=231,y=0;
  
  snake->x=x;
  snake->y=y;
  snake->prev=NULL;
  snake->next=NULL;
  
  for(int i=0;i<19;i++)
  {
    GAME_SnakeAddNode(snake,x+i*TEMP,y);
  }
  GAME_NewFood(GameMap,&Food,FoodBuf[0][0],FoodBuf[0][1]);
  
  GAME_SnakeFillInGameMap(GameMap ,snake);
}

void GAME_SnakeRun(Snake_t *snake)
{
  Snake_t *h=snake,*n=snake->prev;
  Paint_DrawPoint(n->y,n->x, WHITE, DOT_PIXEL_4X4, DOT_STYLE_DFT);
  GAME_SnakePixel(h->x,h->y);
}

unsigned char GAME_SnakeMove(unsigned char (*gamemap)[240],Snake_t *snake,int dir) 
{
  Snake_t *p,*h;
  unsigned char val;
  static int temp=1;
  h=snake;
  p=snake->prev;
  while(p!=snake)
  {
    p->x=p->prev->x;
    p->y=p->prev->y;
    p=p->prev;
  }
  switch(dir)//ESP8266_RxBuf[0])
  {
    case 1:
      h->x-=7;     
    break;
    case 2:
      h->y-=7;
    break;
    case 3:
      h->x+=7;
    break;
    case 4:
      h->y+=7;
    break;
    default:
      h->y+=7;
      break;
  
  }
  
  val=gamemap[h->x][h->y];

  switch(val)
  {
      case 0:       
      break;
    case 1:
      break;
    case 2:
        GAME_SnakeAddNode(snake,h->x,h->y);
         GAME_NewFood(gamemap,&Food,FoodBuf[temp][0],FoodBuf[temp][1]);
        temp++;    
      break;
    default:
      break;
  }
  GAME_SnakeRun(snake);
  GAME_SnakeFillInGameMap(gamemap,snake);
  return 0;
}



void GAME_SnakeFreeNode(Snake_t *snake)
{

}

void GAME_SnakeInit(Snake_t *snake,unsigned char (*gamemap)[240])
{
  Snake_t *p;
  GAME_NewSnake(snake);
  p=snake;
  
  while(p!=NULL)
  {
    GAME_SnakePixel(p->x,p->y);
    p=p->next;
  } 
}
