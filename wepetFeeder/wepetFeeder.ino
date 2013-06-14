#include <SPI.h>
#include <WiFi.h>
#include <Servo.h>

#include <MyCalendar.h>
#include <MyVector.h>

#define ARDUINO_DUE_  //if arudino due
#ifndef ARDUINO_DUE_
// #include <MsTimer2.h>
#else
 #include <Scheduler.h>
 extern void serialEventRun(void) __attribute__((weak));
// void TC3_Handler();
// void TC0_Handler();
// void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);
#endif

#define SECOND 1000
#define CYCLESEC_POLLING 5  //5초
#define CYCLESEC_CHECKEAT 5 //5초
#define CYCLESEC_LCM 5      //최소공배수

#define FEED_NOCHANGED_MAX 36 //5*36=180초 동안 변화 없을시 다 먹음
#define WEIGHT_EATDONE 20   //0점 조절
#define WEIGHT_TOLERANCE 10 //오차허용 그램수 (10그램의변화는 무시)
#define WEIGHT_NEARCLOSE 5 //문 닫기 시작 할 그램 차이

#define LOAD_READING_TIME 100 //ms
#define LOAD_ANALOG_MAX 789
#define LOAD_ANALOG_MIN 38
#define LOAD_WEIGHT_MAX 1000 //g
#define LOAD_WEIGHT_MIN 0 //g
static int LOAD_WEIGHT_ZERO = 270;

#define SERVER_DATA_LINE_NUM 8
#define HTTP_BUF_SIZE 512

static const char* HTTP_OK_STRING="HTTP/1.1 200 OK";

//setupxu
void startSetup();
void checkString(String& input);
String inputString = String();     // a string to hold incoming data
boolean stringComplete = false;    // whether the string is complete

//feeder
void startFeed(int gram);
void eatFeed();

//servomotor control
void openFeeder();
void closeFeeder();

//loadcell
void readLoadcell();
float analogToLoad(float analogval);
float mapfloat(float x, float in_min, float in_max, float out_min, float out_max);
void setZeroPoint();

int parsePolling(const char* buf);

char httpBuf[HTTP_BUF_SIZE];
char bufLine[128];
int httpBufCnt = 0;
void readPrintHttpResponse();
int readHTTPResponse(char* buf);
boolean readLine(const char* src, char* dest);
const char* readLineTo(const char* src, char* dest, int lineNum);

void updateCalendar(const char* strCalendar);

boolean connectWiFi(boolean bFirst);
void disconnectServer();
boolean httpRequest_polling();
boolean httpReqeust_changed();
boolean httpRequest_startEat();
boolean httpRequest_endEat();
boolean httpRequest_getTime();
void printWifiStatus();

void timer_sec();

/***** test *****/
void weightChangeTo(int gram, int ms);
void testFeeding(int gram);
void testEating();
boolean bTestEating = false;
int testRemainWeight = 0;
/***** test *****/

boolean bSetup = false;

//Wifi
char ssid[] = "Hanter Jung's Hotspot"; //  your network SSID (name) 
char pass[] = "68287628";    // your network password (use for WPA, or use as key for WEP)
//char ssid[] = "mr100"; //  your network SSID (name) 
//char pass[] = "A1234567";    // your network password (use for WPA, or use as key for WEP)
//char ssid[] = "AndroidHotspot6790"; //  your network SSID (name) 
//char pass[] = "tpwlssla";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int wifiStatus = WL_IDLE_STATUS;

WiFiClient client;

char server[] = "www.wepet.net";
//IPAddress server(54,249,125,244);

unsigned long lastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean lastConnected = false;                  // state of the connection last time through the main loop
const unsigned long postingInterval = 20*1000;  // delay between updates, in milliseconds

//Feeder
char* SERIAL_NUMBER = "hf1";
MyCalendar calendar;
MyVectorINT16 feedWeightVector(60*5/CYCLESEC_CHECKEAT);  //초기값=5분

Servo servo;
boolean bOpened = false;

int loadcellInput = 0;
int oldFeedWeight = 0;
int nowFeedWeight = 0;
int targetFeedWeight = 500;
int feedNoChangedCnt = 0;
int feedingTimes = 0;

static const int MODE_POLLING = 0;  //폴링 상태
static const int MODE_POLLING_REMAIN = 1;
static const int MODE_WAITING = 2;  //밥 주기 대기 상태
static const int MODE_EATING = 3;   //밥 먹고 있는 상태
int feederMode = MODE_POLLING;
boolean callPolling=false, callStart=false, callEnd=false, callChanged=false, callWaiting=false;

//time
int second = 0;
int timer_count = 0;
boolean bDataUsing = false;

void timer_sec() {
  second++;
  if(second >= 60) {
    second = 0;
    calendar.addMinute();
    Serial.println("addMinute");
  }
  
//  if(!bDataUsing) {
    Serial.print("feedWeight: ");
    Serial.print(oldFeedWeight);
    Serial.print(" -> ");
    Serial.println(nowFeedWeight); 
//  }
//  if(bDataUsing) return;
  
  switch(feederMode) {
  case MODE_POLLING:
    if(timer_count % CYCLESEC_POLLING == 0) {
      if(bDataUsing) break;
      callPolling = true;
    }
    break;
  
  case MODE_POLLING_REMAIN:
    if(timer_count % CYCLESEC_POLLING == 0) {
      if(bDataUsing) break;
      callPolling = true;
    }
    
    if(timer_count % CYCLESEC_CHECKEAT == 0) {
      callChanged = true;   
    }
    break;

  case MODE_WAITING:
    if(timer_count % CYCLESEC_CHECKEAT == 0) {
      callWaiting = true;   
    }
    break;
    
  case MODE_EATING:
    feedingTimes++;
    if(timer_count % CYCLESEC_CHECKEAT == 0) {
      callChanged = true;   
    }
    break;
  }
  
  timer_count++;
  if(timer_count == CYCLESEC_LCM) {
    timer_count = 0;
  }
}


void startSetup() {
  connectWiFi(true);
  
  if(!httpRequest_getTime()) {
    Serial.println("init time httpRequest fail...");
    while(true);
  }
  lastConnected = client.connected();
  
  readHTTPResponse(httpBuf);
  disconnectServer();
  
//  Serial.println(httpBuf);
  
  readLineTo(httpBuf, bufLine, 0);
  if( strcmp(bufLine, HTTP_OK_STRING) ) {
    readLineTo(httpBuf, bufLine, SERVER_DATA_LINE_NUM);
    Serial.print("Init Date : ");
    Serial.print(bufLine);
    updateCalendar(bufLine);
//    calendar.setCalendar(2013, 5, 19, 19, 38);  //DEBUG

    Serial.println(" ..... done.");
    
    char cstr[64];
    calendar.getCaledarStr(cstr);
    Serial.println(cstr);
  } else {
    Serial.println("Init WiFi, Time failed!");
    while(true);
  }
 
  Serial.print("Init timer... ");
#ifdef ARDUINO_DUE_
//  startTimer(TC1, 0, TC3_IRQn, 1);
//  startTimer(TC0, 0, TC0_IRQn, 1);
  Scheduler.startLoop(loop_sec);
  Serial.println("Start with Arduino Due!");
#else
  MsTimer2::set(SECOND, timer_sec);
  MsTimer2::start();
  Serial.println("Start with AVR Arudino!");
#endif
  
  randomSeed(millis());
  
  Serial.println();
  bSetup = true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.flush();
  
  servo.attach(9);
  servo.write(0);
  delay(100);
  
  inputString.reserve(100);
  
    //wifi shield check
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
//    while(true);
  } 
  
  feederMode = MODE_POLLING;
  
  Serial.println("waiting for setup...");
}

void loop() {  
  
  if (stringComplete) {
    checkString(inputString);
    
    //clear the string:
    inputString = "";
    stringComplete = false;
  }
  
  if(!bSetup) return;
  
  //loadcell
  
  
  /*****main code*****/
  //polling
  if(callPolling) {
    callPolling = false;
    while(bDataUsing);
    bDataUsing = true;
    
    Serial.print("polling..... ");
    if(httpRequest_polling()) {
      lastConnected = client.connected();
      
      readHTTPResponse(httpBuf);
      disconnectServer();
      
      readLineTo(httpBuf, bufLine, 0);
      if( strcmp(bufLine, HTTP_OK_STRING) ) {
        readLineTo(httpBuf, bufLine, SERVER_DATA_LINE_NUM);
        
        int weight = parsePolling(bufLine);
        if(weight > 0) {
          targetFeedWeight = weight;
          Serial.print("feed: ");
          Serial.print(targetFeedWeight);
          Serial.println("gram");
          
          startFeed(targetFeedWeight);
        } else {
          Serial.println("not feed");
        }
      } else {
        Serial.println("response fail!");
      }
    } else {
      lastConnected = client.connected();      
      disconnectServer();
      Serial.println("request fail!");
    }
    
    bDataUsing = false;
  }
  
  
  //read LOAD CELL
  readLoadcell();
  

  //waiting
  if(callWaiting) {
    Serial.println("wait eating...");
    callWaiting = false;
    
    while(bDataUsing);
    bDataUsing = true;
    
    //밥을 먹기 시작 한 것임.
    if(oldFeedWeight - nowFeedWeight >= WEIGHT_TOLERANCE) {
      
      while(true) {
        while(!httpRequest_startEat());
        lastConnected = client.connected();
        
        readHTTPResponse(httpBuf);
        disconnectServer();
        
        readLineTo(httpBuf, bufLine, 0);
        if( strcmp(bufLine, HTTP_OK_STRING) ) {
          readLineTo(httpBuf, bufLine, SERVER_DATA_LINE_NUM);
          if(bufLine[0] == '1') {
            Serial.println("success. Puppy Start eating!");
            break;
          } else {
            Serial.println("respose fail... request again...");
            Serial.println(httpBuf);
            break;
          }
          
        } else {
          Serial.println("http request error...");
        }
      }

      httpReqeust_changed();
      lastConnected = client.connected();
      disconnectServer();
      
      feedingTimes = 0;
      
      feedWeightVector.reset();
      feedWeightVector.addItem(oldFeedWeight);  // ==targetWeight
      feedWeightVector.addItem(nowFeedWeight);
      
      oldFeedWeight = nowFeedWeight;

      feederMode = MODE_EATING;
    }
    
    bDataUsing = false;
  }
  
  
  //changed
  if(callChanged) {
    Serial.println("check changed...");
    callChanged = false;
    
    while(bDataUsing);
    bDataUsing = true;
    
    //old - now가 오차범위(10)을 넘을 때
    //무게가 증가하거나 약간만 준 것은 무시함 -> 일정량 이상의 변화만 측정
    if(oldFeedWeight - nowFeedWeight >= WEIGHT_TOLERANCE) {
      
      Serial.print("Send chage weight... ");
      Serial.print(nowFeedWeight);
      Serial.print("gram ... ");
      if(httpReqeust_changed() ) Serial.println("success!");
      else Serial.println("failure.");
      
      readHTTPResponse(httpBuf);
      Serial.println(httpBuf);
      
      lastConnected = client.connected();
      disconnectServer();
      
      if(feederMode == MODE_EATING) {
        feedWeightVector.addItem(nowFeedWeight);
        Serial.print("add Change Weight : ");
        Serial.println(nowFeedWeight);
      }
      
      oldFeedWeight = nowFeedWeight;
      feedNoChangedCnt = 0;
      
      Serial.println("check end");
      
    } else if(feederMode == MODE_EATING) {  //무게 변화가 없음 -> 일정 시간 없으면 다 먹은것임..
      feedNoChangedCnt++;
      Serial.print("noChange... count=");
      Serial.println(feedNoChangedCnt);
      
      if( (feedNoChangedCnt >= FEED_NOCHANGED_MAX) && (feederMode == MODE_EATING) && (nowFeedWeight > WEIGHT_EATDONE) ){
        //서버 전송 에러 시 재전송 문제 해결해야함
        if( httpRequest_endEat() ) {
          lastConnected = client.connected();
          
          readHTTPResponse(httpBuf);
          disconnectServer();
          
          readLineTo(httpBuf, bufLine, 0);
          if( strcmp(bufLine, HTTP_OK_STRING) ) {
            readLineTo(httpBuf, bufLine, SERVER_DATA_LINE_NUM);
            
            if(bufLine[0] == '1') Serial.println("endEat(not all) transmission success.");
            else {
              Serial.println("endEat(not all) response failure.");
              Serial.println(httpBuf);
            }
          }
        } else {
          lastConnected = client.connected();
          disconnectServer();
        }
        
        oldFeedWeight = nowFeedWeight;
        feederMode = MODE_POLLING_REMAIN;
      }
    }
    
    //전부 다 먹었을 경우
    //밥먹는 중일때, WEIGHT_EATDONE(0~15)보다 현재무게가 적으면 0g으로 판단하고 다 먹은 것으로 처리함.
    if( (feederMode == MODE_EATING) && (nowFeedWeight <= WEIGHT_EATDONE) ) {
      Serial.println("EndEat!");
      feedWeightVector.addItem(0);
      
      nowFeedWeight = 0;
      if(httpReqeust_changed() ) Serial.println("success!");
      else Serial.println("failure.");
      
      readHTTPResponse(httpBuf);
      Serial.println(httpBuf);
      
      lastConnected = client.connected();
      disconnectServer();
      
      //서버 전송 에러 시 재전송 문제 해결해야함
      if( httpRequest_endEat() ) {
        lastConnected = client.connected();
        
        readHTTPResponse(httpBuf);
        disconnectServer();
        
        readLineTo(httpBuf, bufLine, 0);
        if( strcmp(bufLine, HTTP_OK_STRING) ) {
          readLineTo(httpBuf, bufLine, SERVER_DATA_LINE_NUM);
          
          if(bufLine[0] == '1') Serial.println("endEat transmission success.");
          else { 
            Serial.println("endEat response failure.");
            Serial.println(httpBuf);
          }
        }
      } else {
        lastConnected = client.connected();
        disconnectServer();
      }
      
      oldFeedWeight = nowFeedWeight = 0;
      feederMode = MODE_POLLING;
    }
    
    bDataUsing = false;
    
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
    Serial.println("setup Feeder");
    startSetup();
  } else if (input == "open" || input == "o") {
    openFeeder();
  } else if (input == "close" || input == "c") {
    closeFeeder();
  } else if (input == "setup") {
    Serial.println("setup WepetFeeder");
  } else if (input == "setZero" || input == "z") {
    setZeroPoint();
  } else {
    char buf[64];
    input.toCharArray(buf, 63);
        
    if(strncmp(buf, "eattest:", 8) == 0) {
      bTestEating = true;
      testRemainWeight = atoi(&buf[8]);
      Serial.print("test start eat with remain weight : ");
      Serial.println(testRemainWeight);
    } else {
      Serial.print("NOT SUPPORT : ");
      Serial.println(buf);
    }
  }
}


#ifdef ARDUINO_DUE_
void serialEventRun(void) {
  if (Serial.available()) serialEvent();
}

void loop_sec() {
  unsigned long oldTime = millis();
  
  timer_sec();
  delay( SECOND - (millis()-oldTime) );  //1초-지난시간
}

//TC1 ch 0
/*void TC3_Handler()
{
  TC_GetStatus(TC1, 0);
  
  timer_sec();
}
//void TC0_Handler()
//{
//  TC_GetStatus(TC0, 0);
//  
//  timer_sec();
//}

//startTimer(TC1, 0, TC3_IRQn, 1);
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
}*/
#endif
