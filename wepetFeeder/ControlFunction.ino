void startFeed(int gram) {  //사료 주기
  openFeeder();
  
  while(true) {
    //read loadcell
//    testFeeding(gram);
    readLoadcell();
    Serial.print("feeding Weight: ");Serial.println(nowFeedWeight);
        
    if( nowFeedWeight >= (targetFeedWeight - WEIGHT_NEARCLOSE) ) {
      oldFeedWeight = nowFeedWeight = targetFeedWeight;
      break;
    }
  }
  
  Serial.println("feeding end");
  closeFeeder();
  feederMode = MODE_WAITING;
}

//UNUSE
void eatFeed() {  //밥을 먹기 시작
}

void readLoadcell() {
//  testEating();

  float analogValueAverage = 0;
  
  long firstTime = millis();
  int cnt = 0;
  while(millis() - firstTime < LOAD_READING_TIME) {
    int analogValue = analogRead(0);
    
    analogValueAverage += analogValue;
    cnt++;
  }
  delay(1);
  
  analogValueAverage = analogValueAverage/cnt;
  
  nowFeedWeight = (int)(analogToLoad(analogValueAverage) - LOAD_WEIGHT_ZERO);
  if(nowFeedWeight < 0) nowFeedWeight = 0;
}

void setZeroPoint() {
  float analogValueAverage = 0;
  
  long firstTimes = millis();
  int cnt = 0;
  while(millis() - firstTimes < LOAD_READING_TIME*2) {
    int analogValue = analogRead(0);
    
    analogValueAverage += analogValue;
    cnt++;
  }
  delay(1);
  
  analogValueAverage = analogValueAverage/cnt;
  
  LOAD_WEIGHT_ZERO = (int)(analogToLoad(analogValueAverage))+2;
  Serial.print("analogValueA: ");Serial.print(analogValueAverage);
  Serial.print("->\tload_zero_weight: ");Serial.println(LOAD_WEIGHT_ZERO);
}

int parsePolling(const char* buf) {  //return 0일시 에는 밥 주지 않음
  if(buf[0] == '1') {
    return atoi(&(buf[2]));
  } else {
    return 0;
  }
}

void openFeeder() {
  if(bOpened) return;
  for(int pos = 0; pos <= 45; pos++) {
    servo.write(pos);
    delay(15);
  }
  delay(100);
  servo.write(45);
  
  Serial.println("open done.");
  bOpened = true;
}

void closeFeeder() {
  if(!bOpened) return;
  for(int pos = 45; pos >= 0; pos--) {
    servo.write(pos);
    delay(15);
  }
  delay(100);
  servo.write(0);
  
  Serial.println("close done.");
  bOpened = false;
}

float analogToLoad(float analogval){
  if(analogval < LOAD_ANALOG_MIN) analogval = LOAD_ANALOG_MIN;
  else if(analogval > LOAD_ANALOG_MAX) analogval = LOAD_ANALOG_MAX;

  // using a custom map-function, because the standard arduino map function only uses int
  float load = mapfloat(analogval, LOAD_ANALOG_MIN, LOAD_ANALOG_MAX, LOAD_WEIGHT_MIN, LOAD_WEIGHT_MAX);
  return load;
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
