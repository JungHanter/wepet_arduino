#include <SD.h>
#include <MsTimer2.h>

#define STEP_ACC_SIZE 40
#define SECOND 1000  //1 sec (1000ms)
#define MINUTE 60000 //1 minute (1000ms * 60sec)

File file;
boolean isFileOpened = false;

String inputString = String();         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

const int LED1 = 9; //red
const int LED2 = 8; //green

boolean bOnLED1 = true;
boolean bOnLED2 = true;

//time
int year, month, day, hour, minute;


//pedometer
int pedoCnt = 0;
int pedoRecordNum = 0;

int old_x = 0;
int old_y = 0;
int old_z = 0;


//timer interrupt routine (using for save)
void timer_sec() {
  
}


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);
  Serial.flush();
  
  inputString.reserve(100);
  
  //SD Card
  Serial.print("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin 
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output 
  // or the SD library functions will not work. 
  pinMode(10, OUTPUT);
  if (!SD.begin(4)) {  //use digital-pin 4,10,11,12,13
    Serial.println("initialization SD failed!");
    return;
  }  
  Serial.println("initialization SD done.");
 
  pedoCnt = 0;
  old_x = analogRead(A0);
  old_y = analogRead(A1);
  old_z = analogRead(A2);
  
  //led set
  bOnLED1 = false;
  pinMode(LED1, OUTPUT);
  digitalWrite(LED1, LOW);
  
  bOnLED2 = false;
  pinMode(LED2, OUTPUT);
  digitalWrite(LED2, LOW);
  
  MsTimer2::set(MINUTE, timer_sec);
  MsTimer2::start();
}

// the loop routine runs over and over again forever:
void loop() {
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
    
    
    //clear the string:
    inputString = "";
    stringComplete = false;
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
    
    bOnLED1 = true;
    digitalWrite(LED1, HIGH);
  
    //위로올라갈때 한번, 내려갈때 한번이므로 짝수일때 step +1
    if(pedoCnt%2 == 0) {
      Serial.print("speed: ");
      Serial.print(acc);
      Serial.print(" \tStepCount: ");
      Serial.println(pedoCnt/2);
    }
  }
  
   
  delay(100);
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
