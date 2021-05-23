#include <WiFi.h>
#include <FS.h>
#include "SPIFFS.h"               // ESP32 only
#include <JPEGDecoder.h>
#include "SPI.h"
#include "TFT_eSPI.h"
#include "fonts/Arial25B.h"
#include "fonts/ARIAL40B.h"
#include "fonts/ARIAL20B.h"
#include "fonts/Arial10.h"

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN     27     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11 
#define ldr        35     // LDR (Light Dependent Resistor)

uint32_t delayMS;
unsigned long lastdhtread = 0;
DHT_Unified dht(DHTPIN, DHTTYPE);

String months[12]={"Jan","Feb","Mar","Apr","May","June","Juy","Aug","Sep","Oct","Nov","Dec"};

TFT_eSPI tft = TFT_eSPI();
unsigned long drawTime = 0;
unsigned long lastTime = 0;
unsigned long timerDelay = 20000;

//const char* ntpServer = "pool.ntp.org";
const char* ntpServer = "1.asia.pool.ntp.org";
const long  gmtOffset_sec = 19800;//3600;
const int   daylightOffset_sec = 3600;

char    time_str[16];
char    date_str[16];
struct tm timeinfo;  
String tstr,dstr;

String s1,s2,s3;
long min_cnt=0;
String batt="0.0";

int last_min=0;
boolean moff = true;

const char* hssid      = "xxxxxxxxxxxxxx";
const char* hpassword  = "1234567890";


String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3]) ; 
}

void WiFi_OFF(){
  Logs("WIFI_OFF");
  delay(500);
  WiFi.disconnect();     
  WiFi.mode(WIFI_OFF); 
}

void WiFi_ON(){
  Logs("WIFI_ON");
  WiFi.mode(WIFI_MODE_APSTA);  
}

void WiFi_Connect(){
  int i=0;
  WiFi_ON();
  WiFi.begin(hssid, hpassword);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      tft.print(".");
      if(i++>10)
        {
          i=0;
          WiFi.reconnect();
          Serial.println("reconnect");        
          tft.println("reconnect");
        }
  }
  tft.println();
  tft.println(String("IP address:")+ IpAddress2String(WiFi.localIP()));
  Serial.println("WiFi connected"); 
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1000);      
}


void getLocalTime()
{
  //struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");  
  tstr="";
  if(timeinfo.tm_hour < 10)
  tstr+="0";
  tstr+=String(timeinfo.tm_hour);
  tstr+=":";  
  if(timeinfo.tm_min < 10)
  tstr+="0";
  tstr+=String(timeinfo.tm_min);
  
  dstr=""+String(timeinfo.tm_mday)+" "+String(months[timeinfo.tm_mon])+" "+String(timeinfo.tm_year+1900);
  tstr.toCharArray(time_str,tstr.length());
  dstr.toCharArray(date_str,dstr.length());
}

float readVoltage()
{
  float vout = 0.0;
  float vin = 0.0;
  float R1 = 4700.0; 
  float R2 = 4700.0;
  int sensorValue = analogRead(34);
  Serial.println(sensorValue);
  vout = (sensorValue * 3.3) / 4095.0; 
  //socket_print("BatADC:"+String(sensorValue)+"  Vout: "+String(vout));  
  
  vout+=0.14;
  //socket_print("BatADC:"+String(sensorValue)+"  Vout: "+String(vout));  
  vin = vout / (R2/(R1+R2)); 
  return (vin);
}

void setup(void) {
  Serial.begin(115200); // Used for messages and the C array generator
  delay(10);
  Serial.println("Digital Clock");
  tft.begin();
  tft.setRotation(3);  // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);  
  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.setFreeFont(&ARIALBD20pt7b);  
  tft.setCursor(80,60);
  tft.print("Nilesh");// Print the string name of the font
  tft.setCursor(30,130);
  tft.print("Digital Frame");// Print the string name of the font
  delay(2000);
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  
  Serial.println("\r\nInitialisation done.");
  listFiles(); // Lists the files so you can see what is in the SPIFFS
  
  tft.fillScreen(TFT_BLACK);            // Clear screen
  tft.setFreeFont(&ARIAL10pt7b);
  tft.setTextSize(1);
  tft.setCursor(0,20);
  tft.print("Connecting to Wifi");// Print the string name of the font
  WiFi_Connect();
  delay(1000);
  dht.begin();    
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);
  delayMS = sensor.min_delay / 1000;
  Serial.print("DelaysMs :");Serial.println(delayMS);
  
  lastdhtread=millis();
  //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "IST-5:30", 1);
  getLocalTime();
  delay(2000);
}

void loop() {
  delay(1);
  if(millis()-lastdhtread > delayMS){    
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    }
    else {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      s1=String(event.temperature,0);
      Serial.println(F("°C"));
    }
    // Get humidity event and print its value.
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    }
    else {
      Serial.print(F("Humidity: "));
      Serial.print(event.relative_humidity);
      s2=String(event.relative_humidity,0);
      Serial.println(F("%"));
    }
    int lv=analogRead(ldr);
    s3=String(map(lv, 0, 4095, 100, 0)).c_str();
    batt=String(readVoltage(),1);
    String logdata = "T: "+s1+" °C H: "+s2+" % L:"+s3+" Battery:"+batt+" V";
    Serial.println(logdata);
    lastdhtread=millis();  
  }

  if((millis() - lastTime) > 1000){   
      getLocalTime();
      if(last_min != timeinfo.tm_min)
      { 
        Serial.println(String("Loading Back"));               
        drawJpeg("/ClockBack.jpg", 0 , 0);     // 240 x 320 image
        update_sensors();
        last_min = timeinfo.tm_min;
      }
      
      tft.setRotation(3);
      tft.setTextColor(TFT_WHITE);
      tft.setFreeFont(&ARIALBD40pt7b);  
      tft.setCursor(1, 60);  
      tft.setTextSize(1); 
      if(timeinfo.tm_hour < 10)
      tft.print("0");
      tft.print(String(timeinfo.tm_hour));
      
      if(moff){
        tft.setTextColor(0x39C4);  
        moff=false;
      }
      else{
        tft.setTextColor(TFT_WHITE);
        moff=true;
      }
      
      tft.print(":");      
      tft.setTextColor(TFT_WHITE);
      if(timeinfo.tm_min < 10)
      tft.print("0");
      tft.print(String(timeinfo.tm_min));
      
      tft.setFreeFont(&ARIALBD20pt7b);
      tft.setTextSize(1);
      tft.setCursor(2, 110); 
      tft.print(String(timeinfo.tm_mday));
      tft.print(" " + months[timeinfo.tm_mon] + " ");
      tft.print(String(timeinfo.tm_year+1900));
      
      lastTime = millis();    
  }
}

void update_sensors(){
  tft.setCursor(115, 170);
  tft.setFreeFont(&ARIALBD25pt7b);
  tft.setTextSize(1);
  tft.print(s1 + " °c");
  tft.setFreeFont(&ARIALBD20pt7b);
  tft.setTextSize(1);
  tft.setCursor(78, 225); 
  tft.print(s2 + "%");
  tft.setCursor(250,225); 
  tft.print(s3);
  tft.setFreeFont(&ARIAL10pt7b);
  tft.setCursor(10,225); 
  tft.print(batt);  
}

void Logs(String str){    
  Serial.println(str);                  
}
