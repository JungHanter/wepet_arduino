int readHTTPResponse(char* buf) {
  while(!client.available());  //wait for client socket
  
  int i = 0;
  httpBufCnt = 0;
  
  while (client.available()) {
    buf[i] = client.read();
    i++;
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
