void startFeed(int gram) {  //사료 주기
  openFeeder();
  
  while(true) {
    //read loadcell
    testFeeding(gram);
        
    if( nowFeedWeight >= (targetFeedWeight - WEIGHT_NEARCLOSE) ) {
      oldFeedWeight = nowFeedWeight = targetFeedWeight;
      break;
    }
  }
  
  closeFeeder();
  feederMode = MODE_WAITING;
}

//UNUSE
void eatFeed() {  //밥을 먹기 시작
}

void readLoadcell() {
  testEating();
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
