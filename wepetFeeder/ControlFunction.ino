
void openFeeder() {
  if(bOpened) return;
  /*for(int pos = 0; pos <= 45; pos++) {
    servo.write(pos);
    delay(10);
  }
  delay(100);*/
  servo.write(45);
  bOpened = true;
}

void closeFeeder() {
  if(!bOpened) return;
  /*for(int pos = 45; pos >= 0; pos--) {
    servo.write(pos);
    delay(10);
  }
  delay(100);*/
  servo.write(0);
  bOpened = false;
}
