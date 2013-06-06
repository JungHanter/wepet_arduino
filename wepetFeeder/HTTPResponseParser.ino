void readPrintHttpResponse() {
  while(!client.available());
  
  while(client.available()) {
    char c = client.read();
    Serial.write(c);
  }
}

int readHTTPResponse(char* buf) {
  while(!client.available());  //wait for client socket
  
  int i = 0;
  httpBufCnt = 0;
  
  while (client.available()) {
    buf[i] = client.read();
    i++;
    
    if(i >= (HTTP_BUF_SIZE-1) ) {
      i = HTTP_BUF_SIZE-1;
      client.flush();
      break;
    }
  }
  buf[i] = '\0';
  
  Serial.println("read response done.");
  return i;
}

boolean readLine(const char* src, char* dest) {
  int i = 0;
  
  while (true) {
    if(src[httpBufCnt] == '\n') {
      dest[i] = '\0';
      httpBufCnt++;
      break;
      
    } else if(src[httpBufCnt] == '\0') {
      dest[i] = '\0';
      httpBufCnt++;
      return false;
      
    } else {
      dest[i++] = src[httpBufCnt++];
    } 
  }
  
  return true;
}


const char* readLineTo(const char* src, char* dest, int lineNum) {
  int srcCnt = 0;
  int lineCnt = 0;
  
  dest[0] = '\0';
  
  if(lineNum != 0) {
    while(true) {
      if(src[srcCnt] == '\n') {
        lineCnt++;
      } else if(src[srcCnt] == '\0') {
        dest[0] = '\0';
        return dest;
      }
      
      srcCnt++;
      if(lineCnt == lineNum) break;
    }
  }
  
  int i = 0;
  while(true) {
    if(src[srcCnt] == '\n') {
      dest[i] = '\0';
      srcCnt++;
      break;
    
    } else {
      dest[i++] = src[srcCnt++];
    }
  }
  
  return dest;
}

void updateCalendar(const char* strCalendar) {
  int pos, chPos;
  boolean bFinish = false;

  int stat = 0; //stat 0(year) ~ 4(minute)
  char chYear[5], chMonth[3], chDay[3], chHour[3], chMinute[3];

  pos = chPos = 0;
  
//  Serial.println();

  while(true) {
    switch (stat) {
    case 0:     //year
      if(strCalendar[pos] == ',') {
//        Serial.print("chPos=");
//        Serial.print(chPos);
//        Serial.print("/pos=");
//        Serial.println(pos);
        
        chYear[chPos] = '\0';

        stat = 1;
        chPos = 0;
      } 
      else {
        chYear[chPos++] = strCalendar[pos];
        
//        Serial.print(chPos-1);
//        Serial.print("=");
//        Serial.println(chYear[chPos-1]);
        
      }
      break;
    case 1:     //month
      if(strCalendar[pos] == ',') {
        chMonth[chPos] = '\0';

        stat = 2;
        chPos = 0;
      } 
      else {
        chMonth[chPos++] = strCalendar[pos];
      }
      break;
    case 2:     //day
      if(strCalendar[pos] == ',') {
        chDay[chPos] = '\0';

        stat = 3;
        chPos = 0;
      } 
      else {
        chDay[chPos++] = strCalendar[pos];
      }
      break;
    case 3:     //hour
      if(strCalendar[pos] == ',') {
        chHour[chPos] = '\0';

        stat = 4;
        chPos = 0;
      } 
      else {
        chHour[chPos++] = strCalendar[pos];
      }
      break;
    case 4:     //minute
      if(strCalendar[pos] == '\0' || strCalendar[pos] == ' ') {
        chMinute[chPos] = '\0';

        stat = -1;
        bFinish = true;
      } 
      else {
        chMinute[chPos++] = strCalendar[pos];
      }
      break;
    }

    if(bFinish) {
      while(bDataUsing);
      bDataUsing = true;

//      Serial.println();
      
//      for(int a=0; a<5; a++) {
//        Serial.print(a);
//        Serial.print("=");
//        Serial.println(chYear[a]);
//      }
//      Serial.println();
//      
//      Serial.print("chYear=");
//      Serial.println(chYear);
//      Serial.println();
      
//      calendar.setCalendar(2013, 6, 4, 17, 50);
      calendar.setCalendar(atoi(chYear), atoi(chMonth), atoi(chDay), atoi(chHour), atoi(chMinute));
      bDataUsing = false;
     
      return;
    }

    pos++;
  }
}
