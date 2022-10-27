#include <ESP8266WiFi.h>        // 本程序使用 ESP8266WiFi库
#include <ESP8266WiFiMulti.h>   //  ESP8266WiFiMulti库
#include <ESP8266WebServer.h>   //  ESP8266WebServer库
//定义帧大小
#define FRAME_SIZE 1536
WiFiEventHandler stationConnectedHandler;  //实例化WIFI事件对象
ESP8266WebServer esp8266_server(80);


const char *ssid = "chn-maker";     // 这里定义将要建立的WiFi名称。
const char *password = "12345678";  // 这里定义将要建立的WiFi密码。此处以12345678为示例

int frame_num = 200;            //动画帧数，缺省值为50
unsigned long frame_time=100;      //每帧时间/ms
unsigned long start_time=0;
unsigned long connect_time;          


const char *headerKeys[] = {"Content-Length", "Content-Type", "Connection", "Date"};
struct dic{                          //mac地址和连接时间的dic结构
  String mac;
  String connect_time;
  };
dic mac_list[6];
int mac_list_size;


void setup() {
  Serial.begin(9600);              // 启动串口通信
  mac_list_size=0;
  WiFi.softAP(ssid, password);     // 此语句是重点。WiFi.softAP用于启动NodeMCU的AP模式。
                                   // 括号中有两个参数，ssid是WiFi名。password是WiFi密码。
                                   // 这两个参数具体内容在setup函数之前的位置进行定义
  Serial.print("Access Point: ");    // 通过串口监视器输出信息
  Serial.println(ssid);              // 告知用户NodeMCU所建立的WiFi名
  Serial.print("IP address: ");      // 以及NodeMCU的IP地址
  Serial.println(WiFi.softAPIP());   // 通过调用WiFi.softAPIP()可以得到NodeMCU的IP地址
  stationConnectedHandler = WiFi.onSoftAPModeStationConnected(onStationConnected);
  if(SPIFFS.begin())
  { // 启动SPIFFS
       Serial.println("SPIFFS Started.");
   }
   else 
   {
      Serial.println("SPIFFS Failed to Start.");
   }
  esp8266_server.begin();                 
  esp8266_server.on("/", handleRoot);    
  esp8266_server.onNotFound(handleNotFound);   
  Serial.println("HTTP esp8266_server started");
  esp8266_server.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));
}

void loop(){
  esp8266_server.handleClient();     // 处理http服务器访问
   
}    

int find_mac(String mac){          //在字典结构中找到目标mac地址的下标
  int index=mac_list_size;
  for(int i=0;i<mac_list_size;i++){ 
    if(mac_list[i].mac==mac){
      index=i;
      break;
    }
  }
  return index;
}

String macToString(const unsigned char* mac) {  //将mac转换成string
  char buf[20];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

String Read(String file_name,int frame_index)  //从闪存系统中读取文件
{
     if(SPIFFS.exists(file_name)){
        Serial.print(file_name);
        Serial.println("  FOUND!");
      }
     else{
        Serial.print(file_name);
        Serial.println("  NOT  FOUND!");
        }
     String data = "";
      File dataFile = SPIFFS.open(file_name, "r");
      dataFile.seek(frame_index*FRAME_SIZE);
      for (int i = 0; i < FRAME_SIZE; i++)
      {
          data += (char)dataFile.read();
      }
      Serial.println("Read finish");
      return data;
} 

void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt) {       //接入点模式下有无线终端设备连接进来时的回调函数
  int index;
  connect_time=millis();
  Serial.print("无线终端设备的mac地址: ");
  Serial.println(macToString(evt.mac));    //调用macToString函数将mac转换成字符串
  index=find_mac(macToString(evt.mac));
  if(index==mac_list_size){
      mac_list_size++;  
  }
  mac_list[index].mac=macToString(evt.mac);
  mac_list[index].connect_time=String(connect_time);
  Serial.print("无线终端设备的序号: ");
  Serial.println(evt.aid);  
}
 
 

void handleRoot(){
  String mac = esp8266_server.arg("mac");
  String file_name = "/" + esp8266_server.arg("Seat_Position");
  Serial.print("begin to hanled a timing request from: ");
  Serial.println(mac);
  Serial.print("seat_position: ");
  Serial.println(file_name);
  int index=find_mac(mac);
  /*String file_name="/";
  for(int i=0;i<=15;i=i+3){
     file_name=file_name+mac[i]+mac[i+1];       
  }
  file_name=file_name+".txt";*/
  esp8266_server.send(200, "application/json", rootJson(file_name,index));   // NodeMCU将调用此函数。
  ESP.wdtFeed();  
  Serial.println("a timing request is handled");
  
}
String rootJson(String file_name,int index){
  String jsonCode = "{\"info\": {\"connect_time\": \"";
  jsonCode = jsonCode+mac_list[index].connect_time;  
  jsonCode += "\",\"frame_time\": \""; 
  jsonCode += String(frame_time);  
  jsonCode += "\",\"start_time\": \""; 
  jsonCode += String(start_time);  
  jsonCode += "\",\"frame_num\": \"";
  File dataFile = SPIFFS.open(file_name, "r");
  frame_num=dataFile.size()/FRAME_SIZE;
  jsonCode += String(frame_num);  
  jsonCode += "\"}}";  
  return jsonCode;
}



void handleNotFound(){   // 当浏览器请求的网络资源无法在服务器找到时，
   String webAddress = esp8266_server.uri();
  
  // 通过handleFileRead函数处处理用户访问
  bool fileReadOK = handleFileRead(webAddress);

  // 如果在SPIFFS无法找到用户访问的资源，则回复404 (Not Found)
  if (!fileReadOK){                                                 
    esp8266_server.send(404, "text/plain", "404 Not Found"); 
  }
}

bool handleFileRead(String path) {            //处理浏览器HTTP访问

  if (path.endsWith("/")) {                   // 如果访问地址以"/"为结尾
       esp8266_server.send(200,"text/plain", "Impossible!");
  } 
  
  String contentType = getContentType(path);  // 获取文件类型
  
  if (SPIFFS.exists(path)) {                     // 如果访问的文件可以在SPIFFS中找到
    File file = SPIFFS.open(path, "r");          // 则尝试打开该文件
    Serial.print("begin to transmit : ");
    Serial.println(path);
    esp8266_server.streamFile(file, contentType);// 并且将该文件返回给浏览器
    Serial.println("Transmit over!");
    file.close();                                // 并且关闭文件
    return true;                                 // 返回true
  }
  return false;                                  // 如果文件未找到，则返回false
}

// 获取文件类型
String getContentType(String filename){
  if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}
void echo_headers() {
 
  if (esp8266_server.hasHeader("Connection")) {//判断该请求头是否存在
    //使用示例,打印当前收集的请求头的数量
    Serial.print("当前请求所收集的请求头数量:"); Serial.println(esp8266_server.headers());
 
    //打印当前请求中所收集的请求头指定项的值
    Serial.print("当前请求所收集的请求头Connection:"); Serial.println(esp8266_server.header("Connection"));
 
    //打印当前请求中所收集的Host
    Serial.print("当前请求所收集的请求头Host :"); Serial.println(esp8266_server.hostHeader());
    //分隔空行
    Serial.println("\r\n");
  }
}
