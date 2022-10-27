#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include "FastLED.h"  
#include <EasyMFRC522.h>
#include <RfidDictionaryView.h>
#include "EasyMFRC522.h"

//ws2812相关声明
#define NUM_LEDS 256
#define DATA_PIN 8
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
uint8_t max_bright=10;
CRGB leds[NUM_LEDS];
int frame_num = 50;    //缺省帧数：50

//视频相关声明
#define FRAME_SIZE 1536
unsigned long frame_time;         //每帧时长

//wifi相关声明
ESP8266WiFiMulti wifiMulti;    // 建立ESP8266WiFiMulti对象
WiFiEventHandler STAConnected; //实例化WIFI事件对象
const char *host = "192.168.4.1"; // 网络服务器IP
const int httpPort = 80;          // http端口80
unsigned long local_time;         //连接到wifi的本地时间
unsigned long connect_time;       //连接到wifi的服务器时间
unsigned long start_time;         //开始时间

//RFID 相关声明
EasyMFRC522 rfidReader(D4, D3); //the MFRC522 reader, with the SDA and RST pins given
                                //the default (factory) keys A and B are used (or used setKeys to change)
byte tagId[4];
byte col[2];
byte row[2];

//mac地址
uint8_t MAC_array_STA[6];
uint8_t MAC_array_AP[6];
char MAC_char_STA[18];
char MAC_char_AP[18];


String Seat_Position;
String URL = "http://192.168.4.1/";
char serialBuffer[FRAME_SIZE];// 建立字符数组用于缓存


int changechar2int(char ch)       //将16进制的字符转为10进制int型
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    else if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
}

String Read(String file_name, int frame_index)      //读取闪存中file_name文件
{
    String data = "";
    File dataFile = SPIFFS.open(file_name, "r");
    dataFile.seek(frame_index*FRAME_SIZE);
    for (int i = 0; i < FRAME_SIZE; i++)
    {
        data += (char)dataFile.read();
    }
    return data;
}

String Read(String file_name)      //读取闪存中file_name文件
{
    String data = "";
    File dataFile = SPIFFS.open(file_name, "r");
    for (int i = 0; i < dataFile.size(); i++)
    {
        data += (char)dataFile.read();
    }
    return data;
}

void WriteBufferToFile( String file_name, char* serialbuffer){
    String tmp="";
    for(int i=0;i<FRAME_SIZE;i++){
      tmp+=serialbuffer[i];  
    }
    File dataFile = SPIFFS.open(file_name, "a");
    dataFile.print(tmp.c_str());
    dataFile.close();  
}
  

void ConnectedHandler(const WiFiEventStationModeConnected &event) //无线终端模式下连接上WIFI时的回调函数
{
    local_time = millis();
    Serial.println("无线终端连接到网络");
}

void display(int state){        //展示第state帧的图像
  //Serial.println(state);
  String data=Read(String("/a.txt"),state); //从文件中读取出第state帧的16进制控制信号
  for(int i=0;i<NUM_LEDS;i++){
    leds[i].r=16 * changechar2int(data[i*6])+changechar2int(data[i*6+1]);
    leds[i].g=16 * changechar2int(data[i*6+2])+changechar2int(data[i*6+3]);
    leds[i].b=16*changechar2int(data[i*6+4])+changechar2int(data[i*6+5]); 
  }
  FastLED.show();
}

void displayloop(){            //同步播放
    unsigned long cur_time = millis();
    int judge = (cur_time - local_time + connect_time) % frame_time;
    if (judge < 0.15*frame_time)
    {
        unsigned long frame = (cur_time - local_time + connect_time - start_time) / frame_time;
        frame = frame % frame_num;
        display(frame);
        delay(0.02 * frame_time);
    }
    else
        delay(0.02 * frame_time);
}

void setup(){
    Serial.begin(9600);
    Serial.println("");
    Serial.println("I am in setup");
    Serial.println("-----step 1 文件系统与LED库初始化-------");
    // 初始化光带 
    LEDS.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);  
    FastLED.setBrightness(max_bright);
    
    // 初始化文件系统
    SPIFFS.begin();
    File dataFile = SPIFFS.open("/a.txt", "w");
    dataFile.close();
    
    // RFID读取位置信息
    rfidReader.init(); // initialization
    delay(1000);
    Serial.println();
    Serial.println("APPROACH a Mifare tag. Waiting...");
    bool success;
    do {
      // if a Mifare tag is detected, returns true and sets the tagId
      success = rfidReader.detectTag(tagId);
      delay(50); //0.05s
    } while (!success);
    //接口：以String类返回位置信息

/*---===========================================================================*/

  int result;
  byte buffer[3*16];

  for (int block = 4; block < 8; block +=4) {
    delay(1000); //1s
    Serial.printf("SECTOR %02d:\n", block/4);

    // reads the next three blocks (corresponds to all space for user data in the sector, except for block #0)
    result = rfidReader.readRaw(block, buffer, 3*16);
    
    if (result < 0) {
      Serial.printf("--> Error: %d.\n\n", result);
      continue;
    }
    row[0]=buffer[0];
    row[1]=buffer[1];
    col[0]=buffer[2];
    col[1]=buffer[3];
  } 
  /*=============================================================================*/

    Seat_Position=String(row[0])+'-'+String(row[1])+'-'+String(col[0])+'-'+String(col[1])+".txt";
    Serial.println("seat:"+ Seat_Position);    
    Serial.println("-----step 1 完成-------");

    
    Serial.println("-----step 2 连接wifi-------");
    STAConnected = WiFi.onStationModeConnected(ConnectedHandler);
    WiFi.macAddress(MAC_array_STA);
    for (int i = 0; i < sizeof(MAC_array_STA); ++i)
    {
        sprintf(MAC_char_STA, "%s%02x:", MAC_char_STA, MAC_array_STA[i]);
    }
    Serial.printf("MAC STA: ");
    Serial.println(MAC_char_STA);
    wifiMulti.addAP("chn-maker", "12345678"); // 将需要连接的一系列WiFi ID和密码输入这里
    Serial.println("Connecting ...");
    int i = 0;
    while (wifiMulti.run() != WL_CONNECTED)
    { // 尝试进行wifi连接。
        delay(500);
        Serial.print(i++);
        Serial.print(' ');
    }
    // WiFi连接成功后将通过串口监视器输出连接成功信息
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID()); // WiFi名称
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP()); // IP
    Serial.println("-----step 2 完成-------");


    Serial.println("-----step 3 连接服务器-------");
    WiFiClient client; // 建立WiFiClient对象
    HTTPClient httpClient;
    Serial.print("Connecting to ");
    Serial.print(host);
    //连接服务器
    if (client.connect(host,httpPort))
    {
        Serial.println("成功连接服务器！");
        //预处理无线终端的mac地址
        for (int i = 0;; i++)
        {
            if (MAC_char_STA[i] == '\0')
            {
                MAC_char_STA[i - 1] = '\0';
                break;
            }
        }
        // 获取并显示服务器响应状态行————>可能存在的问题：并没有确定client收到了http报文
        Serial.println("-----step 3.1 请求同步时间-------");
        String httpRequest = String("GET /?mac=") + String(MAC_char_STA) + "&Seat_Position=" + Seat_Position + " HTTP/1.1\r\n" +
                               "Host: " + host + "\r\n" +
                               "Connection: Keep-Alive\r\n";
                               "\r\n";
        client.print(httpRequest);
        Serial.println("Sending request: ");
        Serial.println(httpRequest);             
        // 获取并显示服务器响应状态行
        while(client.connected()||client.available())
        {
            //Serial.print("等待响应报文...");
            if(client.available())
            {
                String status_response = client.readString();
                Serial.print("获得响应报文: ");
                Serial.println(status_response);
                // 使用find跳过HTTP响应头
                if (client.find("\r\n\r\n"))
                {
                     Serial.println("开始解析json.");
                }
                int Position = status_response.indexOf("\r\n\r\n");
                if(Position!=-1)
                {
                   status_response = status_response.substring(Position+1,status_response.length());
                }
                //解析json       
                DynamicJsonDocument doc(2048);
                deserializeJson(doc, status_response);
                JsonObject info = doc["info"];
                connect_time = info["connect_time"].as<int>();
                frame_time = info["frame_time"].as<int>();
                start_time = info["start_time"].as<int>();
                frame_num = info["frame_num"].as<int>();
                Serial.print("connect_time= "); Serial.println(connect_time);
                Serial.print("frame_time= "); Serial.println(frame_time);
                Serial.print("start_time= "); Serial.println(start_time);
                Serial.print("frame_num= "); Serial.println(frame_num);
          }
        } 
        Serial.println("-----step 3.1 完成-------");
        Serial.println("-----step 3.2 从服务器下载文件------");
        WiFiClient wifiClient;
        HTTPClient httpClient;
        URL=URL+Seat_Position;
        httpClient.begin(URL);
        Serial.print("URL: "); Serial.println(URL);
        int httpCode = httpClient.GET();
        if (httpCode == HTTP_CODE_OK) {
            wifiClient = httpClient.getStream();
            int frame_index=0;
            while (frame_index < frame_num||wifiClient.available()){
                if(wifiClient.available()){
                    wifiClient.readBytes(serialBuffer, FRAME_SIZE);
                    frame_index++;
                    //将Buffer中的信息存储到/a.txt中
                    WriteBufferToFile("/a.txt",serialBuffer);
                    Serial.print("成功存储第");
                    Serial.print(frame_index);
                    Serial.println("帧");
                }
           }
        } 
        else {
            Serial.println("Server Respose Code：");
            Serial.println(httpCode);
        }
        httpClient.end();
        Serial.println("-----step 3.2 完成-------");

    }
    else{
       Serial.println("连接至服务器失败！");
    }
    Serial.println("I am leaving setup!"); 
}



void loop(){
  displayloop();
}
