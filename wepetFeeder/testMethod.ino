/***** test method *****/

void weightChangeTo(int gram, int ms) {
//  nowFeedWeight
}

void testFeeding(int gram) {
  if(nowFeedWeight < gram) {
    nowFeedWeight += random(20,30);
  }
  delay(100);
}


void testEating() {
  if(bTestEating) {
    if(nowFeedWeight > testRemainWeight) {
      nowFeedWeight -= random(0, 2);
    } else {
      bTestEating = false;
    }
  }
  delay(100);
}
