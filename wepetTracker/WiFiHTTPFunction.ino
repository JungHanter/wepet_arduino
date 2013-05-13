void disconnectServer() {
  if (!client.connected()) {
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
  
  
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...SendActivity...");

    client.println("POST /tracker/location/ HTTP/1.1");
    client.println("Host: rhinodream.com");
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
  buf[0] = '0';
  
  strcpy(buf, "serial_number=");
  strcat(buf, SERIAL_NUMBER);
  
  strcat(buf, "&start_time=");
  data.getStartTimeStr(vBuf);
  strcat(buf, vBuf);
  
  strcat(buf, "&end_time=");
  strcat(buf, nowCalendar.getCaledarStr(vBuf));
  
  int hour, minute;
  strcat(buf, "&time=");
  nowCalendar.getElapsedTime(data.getCalendar(), &hour, &minute);
  sprintf(vBuf, "%d,%d", hour, minute);
  strcat(buf, vBuf);
  
  strcat(buf, "&distance=");
  sprintf(vBuf, "%lu", activityDist);
  strcat(buf, vBuf);
  
  dataSize = (unsigned long)strlen(buf) + (unsigned long)(data.getRecordNum())*22;
  
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...SendActivity...");

    client.println("POST /tracker/activity/ HTTP/1.1");
    client.println("Host: rhinodream.com");
    client.println("User-Agent: WepetTracker");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.print(buf);
    
    //locations 전송해야함
    boolean bEnd = false;
    float lat, lon;
    client.print("&locations=");
    while(true) {
      if(data.readNext(&lat, &lon)) {
        bEnd = true;
      }
      
      sprintf(buf, "%10.6f,%10.6f/", lat, lon);
      client.print(buf);
      
      if(bEnd) break;
    }
    
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

boolean httpRequest_getTime() {
  if (client.connect(server, 80)) {
    Serial.println("connecting...GetTime...");

    client.println("POST /time_test/ HTTP/1.1");
    client.println("Host: rhinodream.com");
    client.println("User-Agent: WepetTracker");
    client.println("Connection: close");
    client.println();
    
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

