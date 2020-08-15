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

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t BLEbuf[32] = { 0 };
uint32_t cnt = 0;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


BluetoothSerial SerialBT;

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


#define BILI  0
#define TIME  1
#define GAME  2
#define TEST  3


rmt_data_t led_data[NR_OF_ALL_BITS];

rmt_obj_t* rmt_send_1 = NULL;
rmt_obj_t* rmt_send_2 = NULL;
rmt_obj_t* rmt_send_3 = NULL;
rmt_obj_t* rmt_send_4 = NULL;
rmt_obj_t* rmt_send_5 = NULL;
rmt_obj_t* rmt_send_6 = NULL;

// 定时器
Ticker timer;
Ticker timer_ms;
int count = 0;
boolean flag = true;

// JSON
DynamicJsonBuffer jsonBuffer(256); // ArduinoJson V5

// WiFi 名称与密码
const char *ssid = "2506"; //这里填你家中的wifi名  碳酸镁 OnePlus 6
const char *password = "15814506697";//这里填你家中的wifi密码  cjd84428293  asdfghjkl
// B 站 API 网址: follower, view, likes
String NAME = "DaqoLee";  //改成自己的名字
String UID  = "19577966";  //改成自己的UID 19577966  435162639 483818980
String MyUID = "483818980";
String followerUrl = "http://api.bilibili.com/x/relation/stat?vmid=" + UID;   // 粉丝数
String MyFollowerUrl = "http://api.bilibili.com/x/relation/stat?vmid=" + MyUID;   // 粉丝数
String viewAndLikesUrl = "http://api.bilibili.com/x/space/upstat?mid=" + UID; // 播放数、点赞数
String weatherUrl = "http://api.seniverse.com/v3/weather/now.json?key=Sq4f2lh5b3lLPTCWr&location=guangzhou&language=zh-Hans&unit=c";   // 时间日期


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


typedef struct 
{
  int Hour;
  int Minute;
  int Second;
  uint8_t Flag;
}Clock_t;
Clock_t Clock;
Snake_t SnakeHand;
Food_t Food;

int FoodBuf[][2]=
{
  25,17,
  25,14,
  25,11,
  25,8,
  25,5,
  25,2,

  23,2,
  23,5,
  23,8,
  23,11,
  23,14,

  21,14,
  21,11,
  21,8,
  21,5,
  19,5,
  17,5,
  14,5,
  7,4,
  7,6,
  9,6,
  11,6,
  14,6,
  14,8,
  14,12,
  13,13,
  11,14,
  9,15,
  7,15,
  7,11,
  3,11,
  4,10,
  5,9,
  6,8,
  7,7
  
};

unsigned char GameMap[40][20]={0};

long follower = 0;   // 粉丝数
long view = 0;       // 播放数
long likes = 0;      // 获赞数
int weatherCode=0;
int temperature=0;
uint8_t BtBuff[10]={0};
Adafruit_NeoPixel strip(10, 16, NEO_GRB + NEO_KHZ800);



class MyServerCallbacks : public BLEServerCallbacks {
	void onConnect(BLEServer* pServer) {
		deviceConnected = true;
	};

	void onDisconnect(BLEServer* pServer) {
		deviceConnected = false;
	}
};

class MyCallbacks : public BLECharacteristicCallbacks {
	void onWrite(BLECharacteristic *pCharacteristic) {
		std::string rxValue = pCharacteristic->getValue();

		if (rxValue.length() > 0)
		{
			//        Serial.print("------>Received Value: ");

			for (int i = 0; i < rxValue.length(); i++) {
	//			Serial.print(rxValue[i]);
				rxValue.copy((char*)BLEbuf, rxValue.length(),0);
        Serial.printf("%s\r\n",BLEbuf);
			}
			//        Serial.println();

			if (rxValue.find("A") != -1) {
				//          Serial.print("Rx A!");
			}
			else if (rxValue.find("B") != -1) {
				//          Serial.print("Rx B!");
			}
			//        Serial.println();
		}
	}
};



void setup()
{

  Serial.begin(115200);
 // SerialBT.begin("ESP32"); //Bluetooth device name
  

	// Create the BLE Device
  BLEDevice::init("YingHuo");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();


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
  timer.attach(1, timerCallback); // 每隔1min
  timer_ms.attach_ms(10, timer_msCallback);
  
  //GAME_SnakeInit(&SnakeHand,GameMap);

  // 第一次调用获取数据函数，方便开机即可显示
  getFollower(followerUrl);
  getWeather(weatherUrl);
  getClock();
  //墨水屏初始化
  DEV_Module_Init();
  EPD_2IN13_V2_Init(EPD_2IN13_V2_FULL);
  EPD_2IN13_V2_Clear();
  DEV_Delay_ms(500);
}

int GameDelayFlag=0;

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
#if   1
  Paint_DrawBitMap(gImage_2233); //gImage_66  
  EPD_2IN13_V2_Display(BlackImage);
#endif
//  GlowTubeTest(weaTemp);
  Paint_Clear(WHITE);
  EPD_2IN13_V2_Init(EPD_2IN13_V2_FULL);
  EPD_2IN13_V2_DisplayPartBaseImage(BlackImage);
  EPD_2IN13_V2_Init(EPD_2IN13_V2_PART);
//  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  delay(2000);
 GlowTubeTest(0);
//GAME_SnakeInitTest(&SnakeHand,GameMap);
  Clock.Flag=1;
  while (1)
  {
    

    StatusLoop((uint8_t*)BLEbuf);//BLEbuf BtBuff
    EPD_2IN13_V2_DisplayPart(BlackImage);
    if(GameDelayFlag)
    {
       delay(200);
    }



     if (deviceConnected)
  {
   // memset(BLEbuf, 0, 32);
    //    memcpy(BLEbuf, (char*)"Hello BLE APP!", 32); 
    pCharacteristic->setValue((char*)BLEbuf);

    pCharacteristic->notify(); // Send the value to the app!
  //    Serial.print("*** Sent Value: ");
  //  Serial.print((char*)BLEbuf);
    //    Serial.println(" ***");
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
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
  if (count == 5)
  {
    flag = true; 
    count = 0;
  }
  Clock.Second++;
  if(Clock.Second>=60)
  {  
    
    Clock.Second=0;
    Clock.Minute++;
    if( Clock.Minute>=60)
    {
      Clock.Minute=0;
      Clock.Hour++;
      Clock.Flag=1;
      if(Clock.Hour>=24)
      {
        Clock.Hour=0;
      }
    }  
  }

}

void timer_msCallback()
{
  static int timer_ms_temp=0,timer_ms_num=0;
  
  if(SerialBT.available())
  {
    if(timer_ms_temp == 0)
    {
      timer_ms_num=SerialBT.available();
      //Serial.println(timer_ms_num);
    }
    BtBuff[timer_ms_temp++]=SerialBT.read();
  
  } 
  else
  {
    timer_ms_temp=0;  
  }
}



void StatusLoop(uint8_t * ModeBuf)
{
  long clockData=1000000+Clock.Hour*10000+Clock.Minute*100+Clock.Second;
  static uint8_t N=0,biliFlag=1,timeFlag=0,gameFlag=0,gameDir=4,gameTemp,testFlag=0;
//Serial.printf("%s",ModeBuf);
//Serial.println(ModeBuf);
  if(ModeBuf[0]==0x5A&&ModeBuf[2]==0x5A)
  {
    N=ModeBuf[1];
  }
  else if(ModeBuf[0]==0x55&&ModeBuf[2]==0x55)
  {
    gameDir=ModeBuf[1];
  }
  else if(ModeBuf[0]==0x3A&&ModeBuf[2]==0x3A)
  {
    gameTemp=ModeBuf[1];
  } 
  Serial.printf("\r\n");
  for(int i=0; i<5;i++)
  {
     Serial.print(ModeBuf[i]);
    
    }
 
  switch(N)
  {
    case BILI:
      if(biliFlag)
      {
        biliFlag=0;
        timeFlag=1;
        gameFlag=1;
        testFlag=1;
        if(GameDelayFlag)
        {
          GAME_SnakeFreeNode(&SnakeHand);
        }
        GameDelayFlag=0;
        Paint_Clear(WHITE);
        getFollower(MyFollowerUrl);
        getWeather(weatherUrl);
        WeatherDisplay(weatherCode, temperature);// weaTemp 
        DateDisplay();
        Paint_DrawBitMap_1(0,40,40,120,gImage_bili); 
        Paint_DrawString_CN(0, 220,"萤 火 实 验 室" , &Font12CN, BLACK, WHITE);
      }
     if(flag)
     {  
        flag = false;
        getFollower(MyFollowerUrl);       
        if(Clock.Flag)
        {
          getClock();
          getWeather(weatherUrl);
          WeatherDisplay(weatherCode, temperature);// weaTemp
          DateDisplay();
          Clock.Flag=0;
        }      
     }
    ClockDisplay();    
    GlowTubeDisplay(follower,ModeBuf);
    break;
    
    case TIME: 
      if(timeFlag)
      {
        biliFlag=1;
        timeFlag=0;
        gameFlag=1;
        testFlag=1;
         if(GameDelayFlag)
        {
          GAME_SnakeFreeNode(&SnakeHand);
        }
        GameDelayFlag=0;
        Paint_Clear(WHITE);
        getFollower(MyFollowerUrl);
        getWeather(weatherUrl);
        WeatherDisplay(weatherCode, temperature);// weaTemp
        DateDisplay();
        FollowerDisplay();
        Paint_DrawBitMap_1(0,40,40,120,gImage_bili);  
        Paint_DrawString_CN(0, 220,"萤 火 实 验 室" , &Font12CN, BLACK, WHITE);
      }
     if(flag)
     {  
        flag = false;
        getFollower(MyFollowerUrl);
        FollowerDisplay();
        if(Clock.Flag)
        {
          getClock();
          getWeather(weatherUrl);
          WeatherDisplay(weatherCode, temperature);// weaTemp
          DateDisplay();
          Clock.Flag=0;
        }      
     } 
     GlowTubeDisplay(clockData,ModeBuf);    
    break;    
    
    case TEST:
     if(testFlag)
     {
       biliFlag=1;
       timeFlag=1;
       gameFlag=1;
       testFlag=0;
       if(GameDelayFlag)
       {
         GAME_SnakeFreeNode(&SnakeHand);
       }
       GameDelayFlag=0;
       Paint_Clear(WHITE);
     }
     GlowTubeTest(0);
   
    break;
    case GAME:
      if(gameFlag)
      {
        biliFlag=1;
        timeFlag=1;
        gameFlag=0;
        testFlag=1;
        GameDelayFlag=1;
        Paint_Clear(WHITE);
        GAME_SnakeInit(&SnakeHand,GameMap);
        Paint_DrawBitMap_1(0,40,40,120,gImage_bili);  
        Paint_ClearWindows(0, 220, 0 + Font12CN.Width * 7, 220 + Font12CN.Height, WHITE);
        Paint_DrawString_CN(0, 220,"萤 火 实 验 室" , &Font12CN, BLACK, WHITE);
      }
    //   GAME_SnakeTest();
    
      if(!GAME_SnakeMove(GameMap,&SnakeHand,gameDir))
      {
        N=15;
      }
      
    break;

    case 4:
      if(testFlag)
      {
        biliFlag=1;
        timeFlag=1;
        gameFlag=1;
        testFlag=0;
        if(GameDelayFlag)
       {
         GAME_SnakeFreeNode(&SnakeHand);
       }
       GameDelayFlag=0;
        Paint_Clear(WHITE);
        GAME_SnakeInitTest(&SnakeHand,GameMap);
        Paint_DrawBitMap_1(0,40,40,120,gImage_bili);  
        Paint_ClearWindows(0, 220, 0 + Font12CN.Width * 7, 220 + Font12CN.Height, WHITE);
        Paint_DrawString_CN(0, 220,"萤 火 实 验 室" , &Font12CN, BLACK, WHITE);
      }
       GAME_SnakeTest();
      
     break;

    case 5:
      if(testFlag)
      {
        biliFlag=1;
        timeFlag=1;
        gameFlag=1;
        testFlag=0;
        if(GameDelayFlag)
        {
          GAME_SnakeFreeNode(&SnakeHand);
        }
        GameDelayFlag=0;
        Paint_Clear(WHITE);
        getFollower(followerUrl);
        getWeather(weatherUrl);
        WeatherDisplay(weatherCode, temperature);// weaTemp
        Paint_DrawBitMap_1(0,40,40,120,gImage_bili);  
        DateDisplay();
        Paint_DrawString_CN(0, 220,"萤 火 实 验 室" , &Font12CN, BLACK, WHITE);
      }
     if(flag) 
     {  
        flag = false;
        getFollower(followerUrl);
        if(Clock.Flag)
        {
          getClock();
          Clock.Flag=0;
        }      
     }
    ClockDisplay();    
    GlowTubeDisplay(follower,ModeBuf);
    break;
    
     case 8: 
        if(gameFlag)
        {
          biliFlag=1;
          timeFlag=1;
          gameFlag=0;
          testFlag=1;
          if(GameDelayFlag)
          {
            GAME_SnakeFreeNode(&SnakeHand);
          }
        }
        Paint_Clear(WHITE);
        delay(1000);
        Paint_DrawBitMap(gImage_66); //gImage_66  
//        EPD_2IN13_V2_Display(BlackImage);


      if(gameFlag)
      {
        biliFlag=1;
        timeFlag=1;
        gameFlag=0;
        testFlag=1;
        if(GameDelayFlag)
        {
          GAME_SnakeFreeNode(&SnakeHand);
        }
        GameDelayFlag=0;
        Paint_Clear(WHITE);
        getFollower(followerUrl);
        Paint_DrawBitMap(gImage_66); //gImage_66 
      }
     if(flag)
     {  
        flag = false;
        getFollower(followerUrl);       
        if(Clock.Flag)
        {
          getClock();
          Clock.Flag=0;
        }      
     }
    GlowTubeDisplay(follower,ModeBuf);
       break;
     case 9: 
          if(testFlag)
          {
            biliFlag=1;
            timeFlag=1;
            gameFlag=1;
            testFlag=0;
             if(GameDelayFlag)
            {
              GAME_SnakeFreeNode(&SnakeHand);
            }
            GameDelayFlag=0;
            Paint_Clear(WHITE);
            FollowerDisplay();
            WeatherDisplay(weatherCode, temperature);// weaTemp
            Paint_DrawBitMap_1(0,40,40,120,gImage_bili);  
            DateDisplay();
            Paint_ClearWindows(0, 220, 0 + Font12CN.Width * 7, 220 + Font12CN.Height, WHITE);
            Paint_DrawString_CN(0, 220,"萤 火 实 验 室" , &Font12CN, BLACK, WHITE);
          }
         if(flag)
         {  
            flag = false;
            getFollower(MyFollowerUrl);
            getWeather(weatherUrl);
            FollowerDisplay();
            if(Clock.Flag)
            {
              getClock();
              WeatherDisplay(weatherCode, temperature);// weaTemp
              DateDisplay();
              Paint_DrawBitMap_1(0,40,40,120,gImage_bili);  
              Paint_ClearWindows(0, 220, 0 + Font12CN.Width * 7, 220 + Font12CN.Height, WHITE);
              Paint_DrawString_CN(0, 220,"萤 火 实 验 室" , &Font12CN, BLACK, WHITE);
              Clock.Flag=0;
            }      
         } 
    
         WeatherTest();
          GlowTubeDisplay(clockData,ModeBuf);    
        // GlowTubeDisplay(clockData,ModeBuf);    
        break;    
    
    default:
       if(timeFlag)
      {
        biliFlag=1;
        timeFlag=0;
        gameFlag=1;
        testFlag=1;
     if(GameDelayFlag)
     {
       GAME_SnakeFreeNode(&SnakeHand);
       gameFlag=1;
     }
     }
     GameDelayFlag=1;
     N=GAME_Over(0);
    break;
  }  
  
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

void getClock(void)
{
  now();

  Clock.Hour=hour();
  Clock.Minute=minute();
  Clock.Second=second();
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

void GlowTubeDisplay(int Data , uint8_t * ModeBuf)
{
    char dataBuf[7];
    
    static uint8_t red[8]  ={255,255,255,255,0  ,0  ,0   ,0}; 
    static uint8_t blue[8] ={0  ,0  ,255,0  ,255,255,0   ,0};
    static uint8_t green[8]={0  ,0  ,0  ,255,0  ,255,255 ,0};

    static uint8_t intensity = 4,mode = 1;
    static float temp = 1.0f;

    if(ModeBuf[0] == 0xFF && ModeBuf[2] == 0xFF)
    {
		intensity = ModeBuf[1];
    }
    else if(ModeBuf[0] == 0xAA && ModeBuf[6] == 0xAA)
    {
		mode = ModeBuf[1];
	  if (mode == 1 || mode == 2)
	  {
		  red[ModeBuf[2]] = ModeBuf[3];
		  blue[ModeBuf[2]] = ModeBuf[4];
		  green[ModeBuf[2]] = ModeBuf[5];
	  }
      
    }

    sprintf(dataBuf, "%07d",Data);
    temp=intensity*0.1f;
//    switch(intensity)
//    {
//      case 1: 
//      temp = temp >= 20 ? 20 : temp + 0.2f;
//      break;
//      case 2: 
//      temp = temp <= 1 ? 1 : temp - 0.2f;
//      break;
//      default:
//      break;
//    }

    switch(mode)
    {
      case 1: //整体显示同一个颜色
      for(int i=1;i<=6;i++)
      {
         colWrite(i,  dataBuf[i]-'0',red[0]/temp ,green[0]/temp ,blue[0]/temp);
         delay(2);
      }
      break;

      case 2: //不同管子不同颜色
      for(int i=1;i<=6;i++)
      {
         colWrite(i,  dataBuf[i]-'0',red[i]/temp ,green[i]/temp ,blue[i]/temp);
         delay(2);
      }
      break;
      
      case 3:// 整体光谱循环
      rainbow(Data,1);
      break;
      case 4://全灭 
      for(int i=1;i<=6;i++)
      {
         colWrite(i,  dataBuf[i]-'0',0 ,0 ,0);
         delay(2);
      }
      break;
      default:
      break;
    }
}

void rainbow(int data, int wait)
{
	static uint32_t color = 0xFF0000;
	char dataBuf[7];
	static long firstPixelHue = 0;
	
	sprintf(dataBuf, "%07d", data);
	if (firstPixelHue < 65536)
	{
		int pixelHue = firstPixelHue + (65536L);
		color = strip.gamma32(strip.ColorHSV(pixelHue));
		firstPixelHue += 256;

		for (int i = 1; i <=6; i++)
		{
			colWrite(i, dataBuf[i] - '0', (color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF);
			delay(1);

		}
		delay(wait);

	}
	else
	{
		firstPixelHue = 0;
	}

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

/*
void rainbow(int data, int wait)
{
	static uint32_t color = 0xFF0000;
	char dataBuf[7];
	static long firstPixelHue = 0;

	sprintf(dataBuf, "%d", data);
	if (firstPixelHue < 65536)
	{
		int pixelHue = firstPixelHue + (65536L);
		color = strip.gamma32(strip.ColorHSV(pixelHue));
		firstPixelHue += 256;

		for (int i = 1; i <= 6; i++)
		{
			colWrite(i, dataBuf[i] - '0', (color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF);
			delay(1);
		}
		delay(wait);
	}
	else
	{
		firstPixelHue = 0;
	}
}
*/

void FollowerDisplay(void)
{
    static char flowBuf[7];
    sprintf(flowBuf, "%07d",follower );
    Paint_ClearWindows(5, 140, 5 + Font16_d.Width * 7, 140 + Font16_d.Height, WHITE);
    Paint_DrawString_EN(5, 140, flowBuf, &Font16_d, BLACK, WHITE);
}

void WeatherDisplay(int weaCode, int temp)
{
    Paint_ClearWindows(3, 52, 3 + Font20.Width * 2, 52 + Font20.Height, WHITE);
    Paint_DrawNum(3, 52, temp, &Font20, BLACK, WHITE);
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

void WeatherTest(void)
{
  static int arr[6]={1,4,9,10,21,30};
  static int i=0,mo=8,da=14,we=5;
  char dateBuf[5];

  delay(500);
if(i<6)
{
  WeatherDisplay(arr[i], temperature);// weaTemp
}
if(i>7)
{
  da=da>30?1:da+1;
  we=we>5?0:we+1;
  if(da==1)
  {
    mo++;
  } 
  sprintf(dateBuf, "%02d%02d", mo, da);
  Paint_ClearWindows(0, 110, 0 + Font20.Width * 4, 110 + Font20.Height, WHITE);
  Paint_DrawString_EN(0, 110, dateBuf, &Font20, BLACK, WHITE);
  Paint_ClearWindows(80, 85, 80 + Font24CN.Width, 85 + Font24CN.Height, WHITE);
  Paint_DrawString_CN(80, 85,num_week(we+1,4), &Font24CN, BLACK, WHITE);
  
}
 
  i=i>12?0:i+1;
}

void ClockDisplay()
{
  char timeBuf[10];
    
  sprintf(timeBuf, "%02d %02d", Clock.Hour, Clock.Minute);
  Paint_ClearWindows(1, 135, 1 + Font24.Width * 5+2, 135 + Font24.Height, WHITE);
  Paint_DrawString_EN(1, 135, timeBuf, &Font24, BLACK, WHITE);
  sprintf(timeBuf, "%02d", Clock.Second);
  Paint_DrawString_EN(53, 150, timeBuf, &Font12, BLACK, WHITE);
}

void DateDisplay(void)
{
  char dateBuf[5];
  Paint_ClearWindows(0, 90, 0 + Font20.Width*4, 90 + Font20.Height, WHITE);
  Paint_DrawNum(0, 90,year(), &Font20, BLACK, WHITE);
  sprintf(dateBuf, "%02d%02d", month(), day());
  Paint_ClearWindows(0, 110, 0 + Font20.Width*4, 110 + Font20.Height, WHITE);
  Paint_DrawString_EN(0, 110, dateBuf, &Font20, BLACK, WHITE);
  Paint_ClearWindows(80, 85, 80 + Font24CN.Width, 85 + Font24CN.Height, WHITE);
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
  static int j=0,N=3;
   delay(10);
   if(j<101)
   {
    GAME_SnakeMoveTest(GameMap,&SnakeHand,N);  
   } 
   else
   {
    
     Paint_DrawString_CN(21, 120,"关注＋三连" , &Font12CN,BLACK , WHITE);
   }
   j++;
   if(j==6)
    N=2;
   else if(j==21)
    N=1;
   else if(j==23)
    N=4;
   else if(j==35)
    N=1;
   else if(j==37)
    N=2;
   else if(j==46)
    N=1;
   else if(j==53)
    N=2;
   else if(j==54)
    N=1;
   else if(j==61)
    N=4;
   else if(j==63)
    N=3;
   else if(j==70)
    N=4;
   else if(j==76)
    N=1;
   else if(j==77)
    N=4;
   else if(j==78)
    N=1;
   else if(j==80)
    N=4;  
   else if(j==81)
    N=1; 
   else if(j==83)
    N=4;
   else if(j==84)
    N=1;
   else if(j==86)
    N=2;
   else if(j==90)
    N=1;
   else if(j==94)
    N=2;
   else if(j==95)
    N=3;
   else if(j==96)
    N=2;
   else if(j==97)
    N=3;
   else if(j==98)
    N=2;
   else if(j==99)
    N=3;
   else if(j==100)
    N=2;
   else if(j==101)
    N=3;
}
 

void GAME_SnakeFillInGameMap(unsigned char (*gamemap)[20],Snake_t *snake)
{
  Snake_t *p=snake; 
  while(p->next!=NULL)
  {
    gamemap[p->x][p->y]=1;
    p=p->next;
  }
}

void GAME_SnakePixel(int x,int y,UWORD col)
{
  for(int i=0;i<6;i++)
  {
    for(int j=0;j<6;j++)
    {
      Paint_DrawPoint(6*y+j, 6*x+i, col, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    }
  }
}



void GAME_NewFood(unsigned char (*gamemap)[20],Food_t *food)
{
  while(1)
  {    
    food->x = random(24); 
    food->y = random(18); 
    if(gamemap[food->x][food->y]==0&&food->x>6&&food->y>6)
    {
      gamemap[food->x][food->y]=2;
      GAME_SnakePixel(food->x,food->y,BLACK);
      break;
    }
  }
}
void GAME_NewFoodTest(unsigned char (*gamemap)[20],Food_t *food,int x1,int y1)
{
//  while(1)
  {    
    food->x=x1;
    food->y=y1;
   // if(gamemap[food->x][food->y]==0)
    {
      gamemap[food->x][food->y]=2;
      GAME_SnakePixel(food->x,food->y,BLACK);
 //     break;
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
  int x=5,y=5;
  
  snake->x=x;
  snake->y=y;
  snake->prev=NULL;
  snake->next=NULL;
  
  for(int i=0;i<5;i++)
  {
    GAME_SnakeAddNode(snake,x+i,y);
  }
  GAME_NewFood(GameMap,&Food);
  
  GAME_SnakeFillInGameMap(GameMap ,snake);
}


void GAME_NewSnakeTest(Snake_t *snake)
{
  int x=19,y=17;
  
  snake->x=x;
  snake->y=y;
  snake->prev=NULL;
  snake->next=NULL;
  
  for(int i=0;i<15;i++)
  {
    GAME_SnakeAddNode(snake,x-i,y);
  }
  GAME_NewFoodTest(GameMap,&Food,FoodBuf[0][0],FoodBuf[0][1]);
  
  GAME_SnakeFillInGameMap(GameMap ,snake);
}

void GAME_SnakeRun(unsigned char (*gamemap)[20],Snake_t *snake)
{
  Snake_t *h=snake,*n=snake->prev;
 // Paint_DrawPoint(n->y,n->x, WHITE, DOT_PIXEL_4X4, DOT_STYLE_DFT);
  GAME_SnakePixel(n->x,n->y,WHITE);
  GAME_SnakePixel(h->x,h->y,BLACK);
  gamemap[n->x][n->y]=0;
}

unsigned char GAME_SnakeMove(unsigned char (*gamemap)[20],Snake_t *snake,int dir) 
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
  switch(dir)
  {
    case 1:
      h->x--;     
    break;
    case 2:
      h->y--;
    break;
    case 3:
      h->x++;
    break;
    case 4:
      h->y++;
    break;
    default:
      h->y++;
      break;
  
  }
  
  val=gamemap[h->x][h->y];

  switch(val)
  {
    case 0:       
      break;
    case 1:

         return 0;
      break;
    case 2:
        GAME_SnakeAddNode(snake,h->x,h->y);
        GAME_NewFood(gamemap,&Food);   
      break;
    default:
      break;
  }
  GAME_SnakeRun(gamemap,snake);
  GAME_SnakeFillInGameMap(gamemap,snake);
  return 1;
}


unsigned char GAME_SnakeMoveTest(unsigned char (*gamemap)[20],Snake_t *snake,int dir) 
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
  switch(dir)
  {
    case 1:
      h->x--;     
    break;
    case 2:
      h->y--;
    break;
    case 3:
      h->x++;
    break;
    case 4:
      h->y++;
    break;
    default:
      h->y++;
      break;
  
  }
  
  val=gamemap[h->x][h->y];

  switch(val)
  {
    case 0:       
      break;
    case 1:

        // return 0;
      break;
    case 2:
        GAME_SnakeAddNode(snake,h->x,h->y);
        GAME_NewFoodTest(gamemap,&Food,FoodBuf[temp][0],FoodBuf[temp][1]); 
        temp++;
      break;
    default:
      break;
  }
  Serial.printf("%d\r\n",temp);
  GAME_SnakeRun(gamemap,snake);
  GAME_SnakeFillInGameMap(gamemap,snake);
  return 1;
}


void GAME_SnakeFreeNode(Snake_t *snake)
{
  Snake_t *p,*q;
  p=snake->next;

  while(p)
  {
    q=p->next;
    free(p);
    p=q;  
  }
  snake->next=NULL;
}

void GAME_SnakeInit(Snake_t *snake,unsigned char (*gamemap)[20])
{
  Snake_t *p;
  for(int i=0;i<40;i++)
  {
    for(int j=0;j<20;j++)
    {
      GameMap[i][j]=0;
    }  
  }
  GAME_NewSnake(snake);
  p=snake;
  
  while(p->next!=NULL)
  {
    GAME_SnakePixel(p->x,p->y,BLACK);
    p=p->next;
  }  
  for(int i=0;i<=27;i++)
  {
    for(int j=0;j<=19;j++)
    {
      if(i==0||i==27||j==0||j==19)
      {       
        GAME_SnakePixel(i,j,BLACK); 
        gamemap[i][j]=1;   
      //  Paint_DrawPoint(j, i, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);   
      }
    }
  
  } 
}

void GAME_SnakeInitTest(Snake_t *snake,unsigned char (*gamemap)[20])
{
  Snake_t *p;
  for(int i=0;i<40;i++)
  {
    for(int j=0;j<20;j++)
    {
      GameMap[i][j]=0;
    }  
  }
  GAME_NewSnakeTest(snake);
  p=snake;
  
  while(p->next!=NULL)
  {
    GAME_SnakePixel(p->x,p->y,BLACK);
    p=p->next;
  }  
  for(int i=0;i<=27;i++)
  {
    for(int j=0;j<=19;j++)
    {
      if(i==0||i==27||j==0||j==19)
      {       
        GAME_SnakePixel(i,j,BLACK); 
        gamemap[i][j]=1;   
      //  Paint_DrawPoint(j, i, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);   
      }
    }
  
  } 
}

int GAME_Over(int n)
{
  static int num=0;
  num++;
  if(num==1)
  {
     Paint_ClearWindows(15, 80, 15 + Font12CN.Width*6, 100 + Font12CN.Height, WHITE);
     Paint_DrawString_CN(15, 80,"投币再来" , &Font12CN,WHITE , BLACK);
     Paint_DrawString_CN(80, 82,"▲" , &Font12CN,BLACK , WHITE);
     Paint_DrawString_CN(15, 100,"下次一定" , &Font12CN,BLACK , WHITE);

     return 15;
  }
  else if(num==2)
  {

    delay(1000);
     Paint_ClearWindows(15, 80, 15 + Font12CN.Width*6, 100 + Font12CN.Height, WHITE);
     Paint_DrawString_CN(15, 80,"投币再来" , &Font12CN,BLACK , WHITE);
     Paint_DrawString_CN(80, 102,"▲" , &Font12CN,BLACK , WHITE);
     Paint_DrawString_CN(15, 100,"下次一定" , &Font12CN,WHITE , BLACK);
     return 15;
  }
  else if(num==3)
  {
     delay(1000);
     Paint_ClearWindows(15, 80, 15 + Font12CN.Width*6, 100 + Font12CN.Height, WHITE);
     Paint_DrawString_CN(20, 80,"投币成功！" , &Font12CN,BLACK , WHITE);

     return 15; 
  }
  else if(num==4)
  {
     delay(1000);
     return 4; 
  }
}
