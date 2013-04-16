#include <SD.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <MsTimer2.h>

#define WRITETIME_INTERVER 1000

//init RX2 / TX3
#define RXPIN 2
#define TXPIN 3

#define TERMBAUD  115200
#define GPSBAUD  4800

File settingFile, gpsFile;
boolean isFileOpened = false;

const int RTMODE_NONE = 0;
const int RTMODE_RTSEND = 1;

const int WALKMODE_NONE = 0;
const int WALKMODE_RECORD = 1;

int rtmode = RTMODE_NONE;
int wkmode = WALKMODE_NONE;

String inputString = String();         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

const int LED1 = 9; //red
const int LED2 = 8; //green
boolean bOnLED1 = true;
boolean bOnLED2 = true;

TinyGPS gps;
SoftwareSerial uart_gps(RXPIN, TXPIN);
//SoftwareSerial uart_gps(RXPIN, TXPIN, false);

int gpsRecordNum = 0;

// This is where you declare prototypes for the functions that will be 
// using the TinyGPS library.
void getgps(TinyGPS &gps);



void wrGPSSetFile() {
  settingFile = SD.open("gps.pms", FILE_WRITE);
  
  settingFile.seek(0);
  settingFile.write( (byte)(gpsRecordNum>>8) );
  settingFile.write( (byte)((gpsRecordNum<<8)>>8) );
  settingFile.flush();
  settingFile.close();
}

void timer() {
  bOnLED2 = true;
  digitalWrite(LED2, HIGH);
  
  float latitude, longitude;
  if(uart_gps.available())     // While there is data on the RX pin...
  {
    int c = uart_gps.read();    // load the data into a variable...
    if(gps.encode(c))      // if gps value is valid ...
    { 
      gps.encode(c);
    
      if(rtmode = RTMODE_RTSEND) {
        gps.f_get_position(&latitude, &longitude);
        Serial.print(gpsRecordNum);
        Serial.print(":Lat/Long: "); 
        Serial.print(latitude,5); 
        Serial.print(" / "); 
        Serial.print(longitude,5);
        Serial.print(']');
      }
      if(wkmode = WALKMODE_RECORD) {
        while(isFileOpened);
        isFileOpened = true;
        
        gpsFile = SD.open("gps.pmd", FILE_WRITE);
        gpsFile.print(latitude,5);
        gpsFile.print('/');
        gpsFile.print(longitude,5);
        gpsFile.print(']');
        gpsFile.flush();
        gpsFile.close();
        
        gpsRecordNum++;
        wrGPSSetFile();
        
        isFileOpened = false;
      }
    } else {
      Serial.print("gps is invalid]");
    }
  } else {
    Serial.print("gps is not available]");
  }
}

void setup()
{
  // Sets baud rate of your terminal program
  Serial.begin(TERMBAUD);
  Serial.flush();
  // Sets baud rate of your GPS
  uart_gps.begin(GPSBAUD);
  
//  Serial.println("");
//  Serial.println("GPS Shield QuickStart Example Sketch v12");
//  Serial.println("       ...waiting for lock...           ");
//  Serial.println("");
  
  inputString.reserve(100);
//  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);
  if(!SD.begin(4)) {
//    Serial.println("initialization SD failed!");
  }
  
  isFileOpened = true;
  if(SD.exists("gps.pms")) {
    settingFile = SD.open("gps.pms", FILE_READ);
    gpsRecordNum = ( ((int)(settingFile.read()))<<8 ) + (int)(settingFile.read()) ;
    settingFile.close();
  } else {
    gpsRecordNum = 0;
    wrGPSSetFile();
  }
  isFileOpened = false;
  
//  Serial.println("initialization SD done.");

  bOnLED1 = true;
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, HIGH);
  
  bOnLED2 = false;
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, LOW);
  
  MsTimer2::set(WRITETIME_INTERVER, timer);
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
    if (inputString == "RQ:PMCONN") {  //PM CONNECT
      Serial.print("RS:CONCTD]");
      
    } else if (inputString == "RQ:DISCON") {  //DISCONNECT
      Serial.print("RS:DISCON]");
      
    } else if (inputString == "RQ:GRTGPS") {  //GET REALTIME GPS
      Serial.print("RS:GRTGPS]"); 
      rtmode = RTMODE_RTSEND;
      
    } else if (inputString == "RQ:SRTGPS") {  //STOP RT GPS
      rtmode = RTMODE_NONE;
      
    } else if (inputString == "RQ:STWALK") {  //START WALK
      Serial.print("RS:STWALK]");
      wkmode = WALKMODE_RECORD;
      
    } else if (inputString == "RQ:SPWALK") {  //STOP WALK -> SEND SAVED DATA
      wkmode = WALKMODE_NONE;
      Serial.print("RS:SPWALK]");  //rm ln
      Serial.print(gpsRecordNum);
      Serial.print(']');    //rm ln
      
      while(isFileOpened); //wait for file end;
      isFileOpened = true;
      gpsFile = SD.open("gps.pmd", FILE_READ);
      while (gpsFile.available()) {
        Serial.write(gpsFile.read());
      }
      gpsFile.close();
      Serial.print("RS:GWALKD]");  //SEMD WALK DATA DONE;
      
      //reset record(saved) data file
      gpsRecordNum = 0;
      wrGPSSetFile();
      SD.remove("gps.pmd");
      
      isFileOpened = false;
      
    }
    
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

// The getgps function will get and print the values we want.
void getgps(TinyGPS &gps)
{
  // To get all of the data into varialbes that you can use in your code, 
  // all you need to do is define variables and query the object for the 
  // data. To see the complete list of functions see keywords.txt file in 
  // the TinyGPS and NewSoftSerial libs.
  
  // Define the variables that will be used
  float latitude, longitude;
  // Then call this function
  gps.f_get_position(&latitude, &longitude);
  // You can now print variables latitude and longitude
  Serial.print("Lat/Long: "); 
  Serial.print(latitude,5); 
  Serial.print(", "); 
  Serial.println(longitude,5);
  
  // Same goes for date and time
  int year;
  byte month, day, hour, minute, second, hundredths;
  gps.crack_datetime(&year,&month,&day,&hour,&minute,&second,&hundredths);
  // Print data and time
  Serial.print("Date: "); Serial.print(month, DEC); Serial.print("/"); 
  Serial.print(day, DEC); Serial.print("/"); Serial.print(year);
  Serial.print("  Time: "); Serial.print(hour, DEC); Serial.print(":"); 
  Serial.print(minute, DEC); Serial.print(":"); Serial.print(second, DEC); 
  Serial.print("."); Serial.println(hundredths, DEC);
  //Since month, day, hour, minute, second, and hundr
  
  // Here you can print the altitude and course values directly since 
  // there is only one value for the function
  Serial.print("Altitude (meters): "); Serial.println(gps.f_altitude());  
  // Same goes for course
  Serial.print("Course (degrees): "); Serial.println(gps.f_course()); 
  // And same goes for speed
  Serial.print("Speed(kmph): "); Serial.println(gps.f_speed_kmph());
  //Serial.println();
  
  // Here you can print statistics on the sentences.
  unsigned long chars;
  unsigned short sentences, failed_checksum;
  gps.stats(&chars, &sentences, &failed_checksum);
  //Serial.print("Failed Checksums: ");Serial.print(failed_checksum);
  //Serial.println(); Serial.println();
  
  // Here you can print the number of satellites in view
  Serial.print("Satellites: ");
  Serial.println(gps.satellites());
}
