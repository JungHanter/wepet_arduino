


String inputString = String();         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

const int MODE_RECORD = 0;
const int MODE_RTSEND = 1;

int mode = MODE_RECORD;

//passometer
int old_x = 0;
int old_y = 0;
int old_z = 0;

long passoWalkCnt = 0, passoRunCnt = 0;



// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);
  Serial.flush();
  
  passoWalkCnt = 0;
  passoRunCnt = 0;
  
  old_x = analogRead(A0);
  old_y = analogRead(A1);
  old_z = analogRead(A2);
}

// the loop routine runs over and over again forever:
void loop() {
  
  if (stringComplete) {
    if (inputString == "RQ:PMCONN") {
      Serial.print("RS:CONCTD]");
      
    } else if (inputString == "RQ:DISCON") {
      Serial.print("RS:DISCON]");
      
    } else if (inputString == "RQ:GRTPAS") {
      Serial.print("RS:SRTPAS]");
      mode = MODE_RTSEND;
      
    } else if (inputString == "RQ:STPPAS") {
      mode = MODE_RECORD;
      
    }
  }
    
    
  // read the input on analog pin 0~2:
  int x = analogRead(A0);
  int y = analogRead(A1);
  int z = analogRead(A2);
  int speed = abs(x + y + z - old_x - old_y - old_z);
  old_x = x;
  old_y = y;
  old_z = z;
  
//  Serial.print("speed:");
//  Serial.print(speed);
//  Serial.print("\tmode:");
//  Serial.println(mode);
  
  
  if (speed > 220) {
    passoRunCnt++;
  } else if (speed > 150) {
    passoWalkCnt++;
  }
  
  switch(mode) {
  case MODE_RTSEND:
    if (speed > 220) {
      Serial.print("running: ");
      Serial.print(speed);
      Serial.print(" \trunCnt: ");
      Serial.print(passoRunCnt);
      Serial.println(']');
      
    } else if (speed > 150) {
      Serial.print("walking: ");
      Serial.print(speed);
      Serial.print(" \twalkCnt: ");
      Serial.print(passoWalkCnt);
      Serial.println(']');
    }
    break;
  
  case MODE_RECORD:
    break;
  }
  
  //clear the string:
  inputString = String();
  stringComplete = false;
  
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
