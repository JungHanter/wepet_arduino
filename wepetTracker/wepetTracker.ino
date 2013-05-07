#include <SD.h>

#include <SoftwareSerial.h>
#include <TinyGPS.h>

#include <MsTimer2.h>

#include <SPI.h>
#include <WiFi.h>

#include <MyCalendar.h>
#include <GPSDataFile.h>

#define SECOND 1000

//init RX2 / TX3
#define RXPIN 2
#define TXPIN 3

void printGPS(float& flat, float& flon, unsigned long& age, TinyGPS& gps);

float calcDistance(float lat1, float long1, float lat2, float long2);

boolean connectWiFi();
void httpRequest_sendData(const char* data, int dataSize);
void httpRequest_getTime();
void printWifiStatus();

void timer_sec();

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

const int LED1 = 9; //red    //GPS기록시마디
const int LED2 = 8; //green  //1초마다
boolean bOnLED1 = true;
boolean bOnLED2 = true;

//file DB
GPSData::GPSDataFile data;

//gps
TinyGPS gps;
SoftwareSerial uart_gps(RXPIN, TXPIN);
//SoftwareSerial uart_gps(RXPIN, TXPIN, false);

#define HOME_RADIUS 50
#define CYCLESEC_INSAFE 10
#define CYCLESEC_OUTSAFE 5
#define CYCLESEC_RECORD 5
#define CYCLESEC_LCM 10 //insafe, outsafe, record의 최소공배수

float homeLat = TinyGPS::GPS_INVALID_F_ANGLE, homeLong = TinyGPS::GPS_INVALID_F_ANGLE;
int safeDist = 500;
boolean bInSafe = true;
boolean bActivity = false;

float latitude = 37.503419, longitude = 127.044831;
unsigned long age;


int timer_count = 0;
void timer_sec() {
  bOnLED2 = true;
  digitalWrite(LED2, HIGH);
  
  /*if(uart_gps.available()) {
    char c = uart_gps.read();
    if(gps.encode(c)) {
      gps.f_get_position(&latitude, &longitude, &age);
    }
    printGPS(latitude, longitude, age, gps);
  } else {
    Serial.println("GPS is not available");
  }*/
  latitude += ((float)(random(-200, 200))/1000000.f;
  longitude += ((float)(random(-200, 200))/1000000.f;
  
  
  float nowDist = calcDistance(homeLat, homeLong, latitude, longitude);
  bInSafe = (nowDist <= safeDist);
  
  if(nowDist > HOME_RADIUS) {
    if(bActivity == false) {
      bActivity = true;
      data.startActivity();
    }
    
    if(timer_count % CYCLESEC_RECORD == 0) {
      data.writeGPS(latitude, longitude);
      Serial.println("data Record");
    }
    
    if(bInSafe) {
      if(timer_count % CYCLESEC_INSAFE == 0) {
        //위치전송
        Serial.println("GPS position transmisstion");
      }
    } else {
      if(timer_count % CYCLESEC_OUTSAFE == 0) {
        //위치전송
        Serial.println("GPS position transmisstion");
      }
    }
  } else {
    if(bActivity == true) {
      //액티비티 끝남 -> 데이터 전송
      bActivity = false;
    }
  }
  
  
  timer_count++;
  if(timer_count == CYCLESEC_LCM) {
    timer_count = 0;
  }
}

void setup()
{
  // Sets baud rate of your terminal program
  Serial.begin(115200);
  Serial.flush();
  // Sets baud rate of your GPS
  uart_gps.begin(4800);
  
  Serial.println("Init GPS done. waiting for lock...");
  
  inputString.reserve(100);
//  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
  if(!SD.begin(4)) {
//    Serial.println("initialization SD failed!");
  }
  
  pinMode(10, OUTPUT);
  if (!SD.begin(4)) {  //use digital-pin 4,10,11,12,13
    Serial.println("Init SD failed!");
    return;
  }  
  Serial.println("Init SD done.");

  if (!data.begin()) {
    Serial.println("Init DB failed!");
    return;
  }
  
  data.setCalendar(2013, 4, 21, 11, 07);
  Serial.println("Init  DB done.");

  latitude = longitude = TinyGPS::GPS_INVALID_F_ANGLE;
  age = 0;

  //led set
  bOnLED1 = true;
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, HIGH);
  
  bOnLED2 = false;
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, LOW);
  
  //timer set
  MsTimer2::set(SECOND, timer_sec);
  MsTimer2::start();
}

// This is the main loop of the code. All it does is check for data on 
// the RX pin of the ardiuno, makes sure the data is valid NMEA sentences, 
// then jumps to the getgps() function.
void loop()
{
  if(bOnLED1) {
    bOnLED1 = false;
    digitalWrite(LED1, LOW);
  }
  
  if(bOnLED2) {
    bOnLED2 = false;
    digitalWrite(LED2, LOW);
  }
  
  //request-response
  if (stringComplete) {
    
    
    inputString = "";
    stringComplete = false;
  }
  
  delay(100);
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == ']') {
      stringComplete = true;
    } else {
      // add it to the inputString:
      inputString += inChar;
    } 
  }
}
