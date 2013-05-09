#include <SPI.h>
#include <SD.h>

#include <WiFi.h>

#include <MyCalendar.h>
#include <PedoDataFile.h>

#define ARDUINO_DUE_  //if arudino due
#ifndef ARDUINO_DUE_
// #include <MsTimer2.h>
#else
 extern void serialEventRun(void) __attribute__((weak));
 void TC3_Handler();
 void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq, uint32_t frequency);
#endif

#define LOOP_DELAY 100
#define STEP_ACC_SIZE 40
#define SECOND 1000  //1 sec (1000ms)
#define MINUTE 60000 //1 minute (1000ms * 60sec)

#define CHAR_BUF_SIZE 50

void startSetup();
void checkString(String& input);

const char* getPageDate(PedoData::Page& page, char* buf);
const char* getPageTime(PedoData::Page& page, char* buf);
void getPageSteps(PedoData::Page& page, int* arrStep, int* pCount);
int getPageActives(PedoData::Page& page);

int sendErrorCnt = 0;
void sendAllPedoData();
void printAllPedoData();
void printPage(PedoData::Page& page);
boolean sendPage(PedoData::Page& page);
void transHTTPData(const char* serialNum, PedoData::Page& page, char* buf, int* bufSize);

char httpBuf[512];
int httpBufCnt = 0;
int readHTTPResponse(char* buf);
boolean readLine(const char* src, char* dest);
const char* readLineTo(const char* src, char* dest, int lineNum);

void updateCalendar(const char* strCalendar);

boolean connectWiFi(boolean bFirst);
void disconnectServer();
boolean httpRequest_sendData(const char* data, int dataSize);
boolean httpRequest_getTime();
void printWifiStatus();

void timer_sec();
void timer_min();
void timer_hour();

boolean bSetup = false;

//Wifi
char ssid[] = "Hanter Jung's Hotspot"; //  your network SSID (name) 
char pass[] = "68287628";    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int wifiStatus = WL_IDLE_STATUS;

WiFiClient client;
boolean bSendData = false;

char server[] = "rhinodream.com";

unsigned long lastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean lastConnected = false;                  // state of the connection last time through the main loop
const unsigned long postingInterval = 20*1000;  // delay between updates, in milliseconds

//file DB
PedoData::PedoDataFile data;
boolean bDataUsing = false;

int second = 0, minutes = 0;

//pedometer
char* SERIAL_NUMBER = "11111111";

unsigned int pedoCnt = 0;
unsigned int activeCnt = 0;

int old_x = 0;
int old_y = 0;
int old_z = 0;

boolean bActive = false;

String inputString = String();     // a string to hold incoming data
boolean stringComplete = false;    // whether the string is complete

const int LED9 = 9;
const int LED8 = 8;
boolean bOnLED9 = true;
boolean bOnLED8 = true;

void sendPedo() {
  PedoData::Page page[10];
  int pageSize;

  data.readUsingPages(page, &pageSize, 10);

}

//each sec timer interrupt routine (using for decide active/inactive)
void timer_sec() {
  if(bActive) {
    activeCnt++;
    Serial.print("now second ");
    Serial.print(second);
    Serial.println(" is Active...");
  } else {
    Serial.print("now second ");
    Serial.print(second);
    Serial.println(" is Inctive...");
  }
  bActive = false;

  second++;
  if(second == 3) {
    timer_min();
    second = 0;
  }
}

//each min timer interrupt routine (using for record pedometer Count)
void timer_min() {
  while(bDataUsing);
  bDataUsing = true;
  data.writePedo(pedoCnt/2, activeCnt);
  data.getCalendar().addMinute();
  bDataUsing=false;
  
  Serial.print(minutes);
  Serial.println(" minute... Write Data...");
  Serial.print("steps:");
  Serial.print(pedoCnt/2);
  Serial.print('/');
  Serial.print("actives:");
  Serial.println(activeCnt);
  
  pedoCnt = activeCnt = 0;
  minutes++;
  if(minutes == 60) {
    timer_hour();
    minutes = 0;
  }
}

//each hour timer interrupt routine (using for wifi data transmission)
void timer_hour() {
//  sendAllPedoData();
  bSendData = true;
}

void startSetup() {
  //SD Card
  pinMode(10, OUTPUT);
  if (!SD.begin(4)) {  //use digital-pin 4,10,11,12,13
    Serial.println("Init SD failed!");
    while(true);
  }  
  Serial.println("Init SD done.");

  //data
  if (!data.begin()) {
    Serial.println("Init DB failed!");
    while(true);
  }
//  data.setCalendar(2013, 4, 21, 11, 00);
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

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);
  Serial.flush();
  
  delay(100);
  
  //wifi shield check
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
//    while(true);
  }

  bSetup = false;
  inputString.reserve(100);
  
  second = minutes = 0;

  pedoCnt = activeCnt = 0;
  old_x = analogRead(A0);
  old_y = analogRead(A1);
  old_z = analogRead(A2);

  //led set
  bOnLED9 = false;
  pinMode(LED9, OUTPUT);
  digitalWrite(LED9, LOW);

  bOnLED8 = false;
  pinMode(LED8, OUTPUT);
  digitalWrite(LED8, LOW);
  
  Serial.println("waiting for setup...");
  //startSetup();
}

// the loop routine runs over and over again forever:
void loop() {
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
  
  
  /**** send Data ****/
  if(bSendData) {
    sendAllPedoData();
    bSendData = false;
  }

  /********** check Step **********/
  // read the input on analog pin 0~2:
  int x = analogRead(A0);
  int y = analogRead(A1);
  int z = analogRead(A2);
  int acc = abs(x + y + z - old_x - old_y - old_z);
  old_x = x;
  old_y = y;
  old_z = z;


  if (acc > STEP_ACC_SIZE) {
    pedoCnt++;

    bOnLED9 = true;
    digitalWrite(LED9, HIGH);

    //위로올라갈때 한번, 내려갈때 한번이므로 짝수일때 step +1
    if(pedoCnt%2 == 0) {
      Serial.print("speed: ");
      Serial.print(acc);
      Serial.print(" \tStepCount: ");
      
      Serial.print("StepCount: ");
      Serial.println(pedoCnt/2);

      bActive = true;
    }
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
    Serial.println("setup Pedometer");
    startSetup();
  } else if (input == "sendAllData") {
    sendAllPedoData();
  } else if (input == "printAllData") {
    printAllPedoData();
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


