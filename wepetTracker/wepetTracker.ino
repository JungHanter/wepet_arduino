#include <SPI.h>
#include <SD.h>

#include <WiFi.h>

#include <TinyGPS.h>

#include <MyCalendar.h>
#include <GPSDataFile.h>

#define ARDUINO_DUE_  //if arudino due
#ifndef ARDUINO_DUE_
// #include <MsTimer2.h>
#else
 extern void serialEventRun(void) __attribute__((weak));
 void TC3_Handler();
 void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);
#endif

#define LOOP_DELAY 50
#define SECOND 1000
#define CHAR_BUF_SIZE 50
#define MOVERANGE 50.f

#define SERVER_DATA_LINE_NUM 8
#define HTTP_BUF_SIZE 512

void startSetup();
void checkString(String& input);

void printGPS(float& flat, float& flon, unsigned short failed);
float calcDistance(float lat1, float long1, float lat2, float long2);
boolean setSettings(const char* buf);

char httpBuf[HTTP_BUF_SIZE];
int httpBufCnt = 0;
void readPrintHttpResponse();
int readHTTPResponse(char* buf);
boolean readLine(const char* src, char* dest);
const char* readLineTo(const char* src, char* dest, int lineNum);

void updateCalendar(const char* strCalendar);

boolean connectWiFi(boolean bFirst);
void disconnectServer();
boolean httpRequest_sendLocation(const float& lat, const float& lon);
boolean httpRequest_sendActivity();
boolean httpRequest_getSettings();
boolean httpRequest_getTime();
void printWifiStatus();

void checkGPS();
void timer_sec();

boolean bSetup = false;

//Wifi
char ssid[] = "Hanter Jung's Hotspot"; //  your network SSID (name) 
char pass[] = "68287628";    // your network password (use for WPA, or use as key for WEP)
//char ssid[] = "mr100"; //  your network SSID (name) 
//char pass[] = "A1234567";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int wifiStatus = WL_IDLE_STATUS;

WiFiClient client;

//char server[] = "rhinodream.com";
IPAddress server(54,249,149,48);

unsigned long lastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean lastConnected = false;                  // state of the connection last time through the main loop
const unsigned long postingInterval = 20*1000;  // delay between updates, in milliseconds

String inputString = String();         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

const int LED9 = 9; //red    //GPS기록시마디
const int LED8 = 8; //green  //1초마다
boolean bOnLED9 = true;
boolean bOnLED8 = true;

//file DB
GPSData::GPSDataFile data;
MyCalendar nowCalendar;
boolean bDataUsing = false;

//gps
char* SERIAL_NUMBER = "ht2";

TinyGPS gps;
//SoftwareSerial uart_gps(RXPIN, TXPIN);
//SoftwareSerial uart_gps(RXPIN, TXPIN, false);

#define HOME_RADIUS 50
#define CYCLESEC_INSAFE 10
#define CYCLESEC_OUTSAFE 5
#define CYCLESEC_RECORD 5
#define CYCLESEC_LCM 10 //insafe, outsafe, record의 최소공배수

float homeLat = TinyGPS::GPS_INVALID_F_ANGLE;
float homeLong = TinyGPS::GPS_INVALID_F_ANGLE;
int safeDist = 500;
unsigned short failed;
boolean bInSafe = true;
boolean bActivity = false;
boolean bFirstSafeOut = false;
//char safeOutStatus = 0;  //0=in, 1=firstOut, 2=continuosOut
unsigned long dueTime = 0;

unsigned long activityDist = 0;
float latitude = 37.503419f, oldLatitude = 37.503419f;
float longitude = 127.044831f, oldLongitude = 127.044831f;
unsigned long age;

boolean bGPSCheck = false;
int second = 0;
int timer_count = 0;
void timer_sec() {
  
  bOnLED8 = true;
  digitalWrite(LED8, HIGH);
  
  timer_count++;
  if(timer_count == CYCLESEC_LCM) {
    timer_count = 0;
  }
  
  second++;
  if(second == 60) {
    second = 0;
    nowCalendar.addMinute();
    dueTime++;
  }
  
//  if(bDataUsing) return;

  bGPSCheck = true;
}

void checkGPS() {
  Serial.print("checkGPS... ");
  float nowDist = calcDistance(homeLat, homeLong, latitude, longitude);
  float moveRange = calcDistance(oldLatitude, oldLongitude, latitude, longitude);
  
  boolean newInSafe = (nowDist <= safeDist);
  if( newInSafe == false && bInSafe == true) {
    bFirstSafeOut = true;
  }
  bInSafe = newInSafe;
  
  //  printGPS(latitude, longitude, failed);
  Serial.print("(");
  Serial.print(timer_count);
  Serial.print(")nowDist : ");
  Serial.print(nowDist);
  Serial.print(", ");
  Serial.print(latitude, 6);
  Serial.print("/");
  Serial.print(longitude, 6);
  Serial.print(" / failed=");
  Serial.println(failed);
  
  while(bDataUsing);
  
  if(nowDist > HOME_RADIUS) {
    while(bDataUsing);
    bDataUsing = true;
    if(bActivity == false) {
      bActivity = true;
      data.setCalendar(nowCalendar);
      data.startActivity();
      Serial.println("start act");
      dueTime = 0L;
    }
    
//    if(moveRange >= MOVERANGE) {
      
      if(timer_count % CYCLESEC_RECORD == 0) {
        data.writeGPS(latitude, longitude);
        activityDist += moveRange;
        Serial.println("Record");
      }
      
      if(bInSafe) {
        if(timer_count % CYCLESEC_INSAFE == 0) {
          //위치전송
          httpRequest_sendLocation(latitude, longitude);
          lastConnected = client.connected();
          disconnectServer();
          
          Serial.println("GPS pos tr");
          readPrintHttpResponse();
        }
      } else {
        if(timer_count % CYCLESEC_OUTSAFE == 0) {
          //위치전송
          httpRequest_sendLocation(latitude, longitude);
          lastConnected = client.connected();
          disconnectServer();
          
          Serial.println("GPS pos tr out");
          readPrintHttpResponse();
          
          bFirstSafeOut = false;
        }
      }
//    }
    
    bDataUsing =false;
    
  } else {
    if(bActivity == true) {
      //액티비티 끝남 -> 데이터 전송, 시간동기화?
      Serial.println("end act");
      
      while(bDataUsing);
      bDataUsing = true;
      
      if(data.getRecordNum() > 0) {
        Serial.print("activity data request ... ");
        
        if(!httpRequest_sendActivity()) {
          Serial.println("request fail!");
        } else {
          disconnectServer();
          Serial.println("success!");
          
          readPrintHttpResponse();
        }
        lastConnected = client.connected();
      } else {
        Serial.println("no data...");
      }
      
      data.readDone();
      bDataUsing = false;
      
      activityDist = 0;
      dueTime = 0L;
      bActivity = false;
    }
  }
}

void startSetup() {
  if(bSetup) return;
  
  //SD Card
  pinMode(10, OUTPUT);
  if (!SD.begin(4)) {  //use digital-pin 4,10,11,12,13
    Serial.println("Init SD failed!");
    while(true);
  }  
  Serial.println("Init SD done.");
  
  if (!data.begin()) {
    Serial.println("Init DB failed!");
    while(true);
  }
//  nowCalendar.setCalendar(2013, 4, 21, 11, 07);
//  data.setCalendar(nowCalendar);
  Serial.println("Init DB done.");
  
  //try wifi connect for init time
  connectWiFi(true);
  
  if(!httpRequest_getTime()) {
    Serial.println("init time httpRequest fail...");
    while(true);
  }
  lastConnected = client.connected();
  
  char bufLine[128];
  readHTTPResponse(httpBuf);
  disconnectServer();
  
  Serial.println(httpBuf);
  
  readLineTo(httpBuf, bufLine, 0);
  if( strcmp(bufLine, "HTTP/1.1 200 OK") ) {
    readLineTo(httpBuf, bufLine, SERVER_DATA_LINE_NUM);
    Serial.print("Init Date : ");
    Serial.print(bufLine);
    updateCalendar(bufLine);
//    data.setCalendar(2013, 5, 19, 19, 38);  //DEBUG
//    nowCalendar = data.getCalendar();
    Serial.println(" ..... done.");
  } else {
    Serial.println("Init WiFi, Time failed!");
    while(true);
  }
  
  /*** settings ***/
  if(!httpRequest_getSettings()) {
    Serial.println("init settings httpRequest fail...");
    while(true);
  }
  lastConnected = client.connected();
  
  readHTTPResponse(httpBuf);
  disconnectServer();
  
//  Serial.println("*************settings");
//  Serial.println(httpBuf);
  
  readLineTo(httpBuf, bufLine, 0);
  if( strcmp(bufLine, "HTTP/1.1 200 OK") ) {
    readLineTo(httpBuf, bufLine, SERVER_DATA_LINE_NUM);
    Serial.print("read Setting Data : ");
    Serial.println(bufLine);

    setSettings(bufLine);
    Serial.print("settings : ");
    Serial.print(homeLat, 6);
    Serial.print('/');
    Serial.print(homeLong, 6);
    Serial.print(" / ");
    Serial.print(safeDist);

    Serial.println(" ..... done.");
  } else {
    Serial.println("Init WiFi, Settings failed!");
    while(true);
  }
  
  //init lat/long
  latitude = homeLat;
  longitude = homeLong;
  
  randomSeed(millis());
    
  Serial.print("Init timer... ");
#ifdef ARDUINO_DUE_
  startTimer(TC1, 0, TC3_IRQn, 1);
  Serial.println("Start with Arduino Due!");
#else
  MsTimer2::set(SECOND, timer_sec);
  MsTimer2::start();
  Serial.println("Start with AVR Arudino!");
#endif

  Serial.println();
  bSetup = true;
}

void setup()
{
  // Sets baud rate of your terminal program
  Serial.begin(115200);
  Serial.flush();
  
  // Sets baud rate of your GPS
  Serial1.begin(4800);
  Serial.println("Init GPS done. waiting for lock...");
  
  delay(100);
    
  //wifi shield check
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
//    while(true);
  } 
  
  bSetup = false;
  inputString.reserve(100);

//  latitude = longitude = TinyGPS::GPS_INVALID_F_ANGLE;
  homeLat = latitude;
  homeLong = longitude;
  age = 0;

  //led set
  bOnLED9 = true;
  pinMode(LED9, OUTPUT);
  digitalWrite(LED9, HIGH);
  
  bOnLED8 = false;
  pinMode(LED8, OUTPUT);
  digitalWrite(LED8, LOW);

  Serial.println("waiting for setup...");
  
  delay(500);
  startSetup();
}


void loop()
{
  //request-response
  if (stringComplete) {
    checkString(inputString);
    
    //clear the string:
    inputString = "";
    stringComplete = false;
  }
  
  if(!bSetup) return;
  
  //LED off
  if(bOnLED9) {
    bOnLED9 = false;
    digitalWrite(LED9, LOW);
  }
  
  if(bOnLED8) {
    bOnLED8 = false;
    digitalWrite(LED8, LOW);
  }
  
  oldLatitude = latitude;
  oldLongitude = longitude;
  
  /*
  boolean newData = false;
  unsigned long start = millis();
  while (millis() - start < 1000)
  {
    while (Serial1.available())
    {
      if (gps.encode(Serial1.read()))
        newData = true;
    }
  }
  
  if(newData) {
    float flat, flon;
    unsigned long age;
    unsigned long chars;
    unsigned short sentences;
    gps.f_get_position(&flat, &flon, &age);
    gps.stats(&chars, &sentences, &failed);
    
    if(flat == TinyGPS::GPS_INVALID_F_ANGLE || flon == TinyGPS::GPS_INVALID_F_ANGLE) {
      //위경도 변화없음
      failed = 100;
    } else {
      latitude = flat;
      longitude = flon;
    }
  } else {
    //위경도 변화없음
    failed = 100;
  }*/

  
  //test move data
  delay(1000);
  latitude += ( (float)(random(-200,200)) / 1000000.f );
  longitude += ( (float)(random(-200,200)) / 1000000.f );
  
  checkGPS();
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      // add it to the inputString:
      inputString += inChar;
    } 
  }
}

void checkString(String& input) {
  if(input == "setup") {
    Serial.println("setup GPSTracker");
    startSetup();
  }
}

#ifdef ARDUINO_DUE_
void serialEventRun(void) {
  if (Serial.available()) serialEvent();
}

//TC1 ch 0
void TC3_Handler()
{
  TC_GetStatus(TC1, 0);
  
  timer_sec();
}

void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency) {
  //frequency 1 per 1sec
  //frequency 5 per 200ms
  //frequency 10 per 100ms
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk((uint32_t)irq);
  TC_Configure(tc, channel, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK4);
  uint32_t rc = VARIANT_MCK/128/frequency; //128 because we selected TIMER_CLOCK4 above
  TC_SetRA(tc, channel, rc/2); //50% high, 50% low
  TC_SetRC(tc, channel, rc);
  TC_Start(tc, channel);
  tc->TC_CHANNEL[channel].TC_IER=TC_IER_CPCS;
  tc->TC_CHANNEL[channel].TC_IDR=~TC_IER_CPCS;
  NVIC_EnableIRQ(irq);
}
#endif

