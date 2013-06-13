#include <SPI.h>
#include <SD.h>


#include <MyCalendar.h>
#include <MyVector.h>
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

#define SERVER_DATA_LINE_NUM 8

#define HTTP_BUF_SIZE 512
#define CHAR_BUF_SIZE 50

void startSetup();
void checkString(String& input);

const char* getPageDate(PedoData::Page& page, char* buf);
const char* getPageTime(PedoData::Page& page, char* buf);
void getPageSteps(PedoData::Page& page, int* arrStep, int* pCount);
int getPageActives(PedoData::Page& page);

void printAllPedoData();
void printPage(PedoData::Page& page);

void updateCalendar(const char* strCalendar);


void timer_sec();
void timer_min();
void timer_hour();

boolean bSetup = false;


//file DB
PedoData::PedoDataFile data;
boolean bDataUsing = false;

int second = 0, minutes = 0;

//pedometer
char* SERIAL_NUMBER = "hm2";

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
  if(second == 10) {
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

  //data
  if (!data.begin()) {
    Serial.println("Init DB failed!");
    while(true);
  }
  data.reset();
  Serial.println("Init DB done.");
  
  data.setCalendar(2013, 6, 7, 19, 0);  

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
  
  delay(500);
  startSetup();
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


