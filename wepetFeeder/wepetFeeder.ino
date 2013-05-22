#include <Servo.h>

Servo servo;

//setup
void checkString(String& input);
String inputString = String();     // a string to hold incoming data
boolean stringComplete = false;    // whether the string is complete

//servomotor control
void openFeeder();
void closeFeeder();

boolean bOpened = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.flush();
  
  servo.attach(9);
  servo.write(0);
  
  inputString.reserve(100);
}

void loop() {
  // put your main code here, to run repeatedly: 
  if (stringComplete) {
    checkString(inputString);
    
    //clear the string:
    inputString = "";
    stringComplete = false;
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
  } else if (input == "open") {
    openFeeder();
  } else if (input == "close") {
    closeFeeder();
  }
}
