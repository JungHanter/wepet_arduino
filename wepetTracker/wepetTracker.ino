#include <SPI.h>
#include <SD.h>

#include <WiFi.h>

#include <SoftwareSerial.h>
#include <TinyGPS.h>

#include <MyCalendar.h>
#include <GPSDataFile.h>

//#define ARDUINO_DUE_  //if arudino due
#ifndef ARDUINO_DUE_
 #include <MsTimer2.h>
#else
 extern void serialEventRun(void) __attribute__((weak));
 void TC3_Handler();x
 void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);
#endif

#define LOOP_DELAY 50
#define SECOND 1000
#define CHAR_BUF_SIZE 50

//init RX2 / TX3
#define RXPIN 2
#define TXPIN 3

void startSetup();
void checkString(String& input);

void printGPS(float& flat, float& flon, unsigned long& age, TinyGPS& gps);
float calcDistance(float lat1, float long1, float lat2, float long2);

char httpBuf[512];
int httpBufCnt = 0;
int readHTTPResponse(char* buf);
boolean readLine(const char* src, char* dest);
const char* readLineTo(const char* src, char* dest, int lineNum);

void updateCalendar(const char* strCalendar);

boolean connectWiFi(boolean bFirst);
void disconnectServer();
boolean httpRequest_sendLocation(const float& lat, const float& lon);
boolean httpRequest_sendActivity();
boolean httpRequest_getTime();
void printWifiStatus();

void checkGPS();
void timer_sec();

boolean bSetup = false;

//Wifi
char ssid[] = "Hanter Jung's Hotspot"; //  your network SSID (name) 
char pass[] = "68287628";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int wifiStatus = WL_IDLE_STATUS;

WiFiClient client;

char server[] = "rhinodream.com";

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
char* SERIAL_NUMBER = "11111111";

TinyGPS gps;
SoftwareSerial uart_gps(RXPIN, TXPIN);
//SoftwareSerial uart_gps(RXPIN, TXPIN, false);

#define HOME_RADIUS 50
#define CYCLESEC_INSAFE 10
#define CYCLESEC_OUTSAFE 5
#define CYCLESEC_RECORD 5
#define CYCLESEC_LCM 10 //insafe, outsafe, record의 최소공배수

float homeLat = TinyGPS::GPS_INVALID_F_ANGLE;
float homeLong = TinyGPS::GPS_INVALID_F_ANGLE;
int safeDist = 500;
boolean bInSafe = true;
boolean bActivity = false;

unsigned long activityDist = 0;
float latitude = 37.503419f;
float longitude = 127.044831f;
unsigned long age;

boolean bGPSCheck = false;
int second = 0;
int timer_count = 0;
void timer_sec() {
  
  bOnLED8 = true;
  digitalWrite(LED8, HIGH);
  
  timer_count++;
  second++;
  if(second == 60) {
    second = 0;
    nowCalendar.addMinute();
  }
  
  /*if(uart_gps.available()) {
    char c = uart_gps.read();
    if(gps.encode(c)) {
      gps.f_get_position(&latitude, &longitude, &age);
    }
    printGPS(latitude, longitude, age, gps);
  } else {
    Serial.println("GPS is not available");
  }*/
  //test move data
  latitude += ( (float)(random(-200,200)) / 1000000.f );
  longitude += ( (float)(random(-200,200)) / 1000000.f );

  bGPSCheck = true;
}

void checkGPS() {
  float nowDist = calcDistance(homeLat, homeLong, latitude, longitude);
  bInSafe = (nowDist <= safeDist);
  
  //  printGPS(latitude, longitude, age, gps);
  Serial.print("(");
  Serial.print(timer_count);
  Serial.print(")nowDist : ");
  Serial.print(nowDist);
  Serial.print(", ");
  Serial.print(latitude, 6);
  Serial.print("/");
  Serial.println(longitude, 6);
    
  if(nowDist > HOME_RADIUS) {
    if(bActivity == false) {
      bActivity = true;
      data.setCalendar(nowCalendar);
      data.startActivity();
      Serial.println("start");
    }
    
    if(timer_count % CYCLESEC_RECORD == 0) {
      data.writeGPS(latitude, longitude);
      Serial.println("Record");
    }
    
    if(bInSafe) {
      if(timer_count % CYCLESEC_INSAFE == 0) {
        //위치전송
        httpRequest_sendLocation(latitude, longitude);
        lastConnected = client.connected();
        Serial.println("GPS pos tr");
      }
    } else {
      if(timer_count % CYCLESEC_OUTSAFE == 0) {
        //위치전송
        httpRequest_sendLocation(latitude, longitude);
        lastConnected = client.connected();
        Serial.println("GPS pos tr out");
      }
    }
  } else {
    if(bActivity == true) {
      //액티비티 끝남 -> 데이터 전송, 시간동기화?
      Serial.print("activity data request ... ");
      if(!httpRequest_sendActivity()) {
        Serial.println("request fail!");
      }
      lastConnected = client.connected();
      
      data.readDone();
      Serial.println("end");
      bActivity = false;
    }
  }
  
  if(timer_count == CYCLESEC_LCM) {
    timer_count = 0;
  }
  
}

void startSetup() {
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
  nowCalendar.setCalendar(2013, 4, 21, 11, 07);
  data.setCalendar(nowCalendar);
  Serial.println("Init DB done.");
  
  //try wifi connect
  connectWiFi(true);
  if(!httpRequest_getTime()) {
    Serial.println("init time httpRequest fail...");
    while(true);
  }
  lastConnected = client.connected();
  
  char bufLine[128];
  readHTTPResponse(httpBuf);
  disconnectServer();
  
  readLineTo(httpBuf, bufLine, 0);
  if( strcmp(bufLine, "HTTP/1.1 200 OK") ) {
    readLineTo(httpBuf, bufLine, 9);
    Serial.print("Init Date : ");
    Serial.println(bufLine);
    updateCalendar(bufLine);
  } else {
    Serial.println("Init WiFi, Time failed!");
    while(true);
  }
  
    
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
  uart_gps.begin(4800);
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
}


void loop()
{
  delay(LOOP_DELAY);
  
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
  
  if(bGPSCheck) {
    bGPSCheck = false;
    checkGPS();
  }
  
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

