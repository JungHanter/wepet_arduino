void disconnectServer() {
  if (!client.connected()) {
    client.flush();
    client.stop();
  }
}

boolean connectWiFi(boolean bFirst) {
  while ( wifiStatus != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    wifiStatus = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  
  printWifiStatus();
  
//  disconnectServer();

  return true;
}

boolean httpRequest_sendLocation(const float& lat, const float& lon) {
  unsigned long dataSize = 0;
  char buf[128], vBuf[16];
  buf[0] = '0';
  
  strcpy(buf, "serial_number=");
  strcat(buf, SERIAL_NUMBER);
  
  strcat(buf, "&date_time=");
  strcat(buf, nowCalendar.getCaledarStr(vBuf));
  
  strcat(buf, "&latitude=");
  sprintf(vBuf, "%10.6f", lat);
  strcat(buf, vBuf);
  
  strcat(buf, "&longitude=");
  sprintf(vBuf, "%10.6f", lon);
  strcat(buf, vBuf);
  
  dataSize = strlen(buf);
  
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...SendLocation...");
    
    //DEBUG
    Serial.print("data : ");
    Serial.println(buf);

    client.println("POST /hw/tracker/location/ HTTP/1.1");
    client.println("Host: 54,249,149,48");
    client.println("User-Agent: WepetTracker");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.print(buf);
    
    // note the time that the connection was made:
    lastConnectionTime = millis();
    return true;
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println("disconnecting.");
    client.stop();
    
    return false;
  }
}

boolean httpRequest_sendActivity() {
  unsigned long dataSize = 0;
  
//  char buf[192], vBuf[32];
  char buf[128], vBuf[32];
  char latbuf[16], lonbuf[16];
  buf[0] = '0';
  
  strcpy(buf, "serial_number=");
  strcat(buf, SERIAL_NUMBER);
  
  strcat(buf, "&date_time=");
  data.getStartTimeStr(vBuf);
  strcat(buf, vBuf);
  
  int hour, minute;
  strcat(buf, "&time=");
  nowCalendar.getElapsedTime(data.getCalendar(), &hour, &minute);
  sprintf(vBuf, "%d,%d", hour, minute);
  strcat(buf, vBuf);
  
  strcat(buf, "&distance=");
  sprintf(vBuf, "%lu", activityDist);
  strcat(buf, vBuf);
  
  //24 -> -xxx.xxxxxx,-xxx.xxxxxx/
  dataSize = (unsigned long)strlen(buf) + (unsigned long)(data.getRecordNum())*24 + 1;
  
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...!SendActivity...");
    
    //DEBUG
    Serial.print("data_header : ");
    Serial.println(buf);

    client.println("POST /hw/tracker/activity/ HTTP/1.1");
    client.println("Host: 54,249,149,48");
    client.println("User-Agent: WepetTracker");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.print(buf);
    Serial.print(6);
    
    //locations 전송해야함
    boolean bEnd = false;
    float lat, lon;
    client.print("&locations=");
    while(true) {
      if(data.readNext(&lat, &lon)) {
        bEnd = true;
      }
      
      //DEBUG
      Serial.print(7);
      sprintf(buf, "%11.6f,%11.6f/", lat, lon);
      client.print(buf); //not
      Serial.print(8);
      Serial.print(' ');
      Serial.print(buf);
      Serial.print('/');
      
      if(bEnd) break;
    }
    client.print('\0');
    
    // note the time that the connection was made:
    lastConnectionTime = millis();
    return true;
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println("disconnecting.");
    client.stop();
    
    return false;
  }
}

boolean httpRequest_getSettings() {
  unsigned long dataSize = 0;
  char buf[128];
  buf[0] = '0';
  
  strcpy(buf, "serial_number=");
  strcat(buf, SERIAL_NUMBER);
  
  dataSize = strlen(buf);
  
  if (client.connect(server, 80)) {
    Serial.println("connecting...GetTime...");

    client.println("POST /hw/tracker/safe_zone_info/ HTTP/1.1");
    client.println("Host: 54,249,149,48");
    client.println("User-Agent: WepetTracker");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.println(buf);
    
    // note the time that the connection was made:
    lastConnectionTime = millis();
    return true;
  } 
  else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println("disconnecting.");
    client.stop();
    return false;
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

