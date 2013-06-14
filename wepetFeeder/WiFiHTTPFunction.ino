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

boolean httpRequest_polling() {
  unsigned long dataSize = 0;
  char buf[32];
  buf[0] = '0';
  
  strcpy(buf, "serial_number=");
  strcat(buf, SERIAL_NUMBER);
  
  dataSize = strlen(buf);
  
  if (client.connect(server, 80)) {
    
    client.println("POST /hw/feeder/polling/ HTTP/1.1");
    client.println("Host: www.wepet.net");
    client.println("User-Agent: WepetFeeder");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.println(buf);
    
    // note the time that the connection was made:
    lastConnectionTime = millis();
    return true;
  } else {
    // if you couldn't make a connection:
    client.stop();
    return false;
  }
}

boolean httpReqeust_changed() {
  unsigned long dataSize = 0;
  char buf[64], vBuf[16];
  buf[0] = '0';
  
  strcpy(buf, "serial_number=");
  strcat(buf, SERIAL_NUMBER);
  
  strcat(buf, "&left_quantity=");
  sprintf(vBuf, "%d", nowFeedWeight);
  strcat(buf, vBuf);
  
  dataSize = strlen(buf);
  
  if (client.connect(server, 80)) {
    
    client.println("POST /hw/feeder/bowl/ HTTP/1.1");
    client.println("Host: www.wepet.net");
    client.println("User-Agent: WepetFeeder");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.println(buf);
    
    // note the time that the connection was made:
    lastConnectionTime = millis();
    return true;
  } else {
    // if you couldn't make a connection:
    client.stop();
    return false;
  }
}

boolean httpRequest_startEat() {
  unsigned long dataSize = 0;
  char buf[128], vBuf[32];
  buf[0] = '0';
  
  strcpy(buf, "serial_number=");
  strcat(buf, SERIAL_NUMBER);
  
  strcat(buf, "&start_time=");
  strcat(buf, calendar.getCaledarStr(vBuf));
  
  dataSize = strlen(buf);
  
  Serial.print("connecting...startEat...");
  if (client.connect(server, 80)) {
    
    client.println("POST /hw/feeder/start/ HTTP/1.1");
    client.println("Host: www.wepet.net");
    client.println("User-Agent: WepetFeeder");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.println(buf);
    
    Serial.print("sendData : ");
    Serial.println(buf);
    
    // note the time that the connection was made:
    lastConnectionTime = millis();
    Serial.println("Success");
    return true;
  } else {
    // if you couldn't make a connection:
    client.stop();
    Serial.println("failure");
    return false;
  }
}

boolean httpRequest_endEat() {
  unsigned long dataSize = 0;
  int gramCnt = feedWeightVector.getSize();
  char buf[128], vBuf[32];
  buf[0] = '0';
  
  strcpy(buf, "serial_number=");
  strcat(buf, SERIAL_NUMBER);
  
  strcat(buf, "&end_time=");
  strcat(buf, calendar.getCaledarStr(vBuf));
  
  strcat(buf, "&time=");
  sprintf(vBuf, "%d", feedingTimes);
  strcat(buf, vBuf);
  
  strcat(buf, "&grams=");
  
  
  dataSize = strlen(buf) + ( gramCnt*4 -1 );  //comma + %3d
  
  Serial.print("connecting...endEat...");
  if (client.connect(server, 80)) {
    
    client.println("POST /hw/feeder/end/ HTTP/1.1");
    client.println("Host: www.wepet.net");
    client.println("User-Agent: WepetFeeder");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.print(buf);
    
    
    //debugging
    Serial.print("senddata : ");
    Serial.print(buf);
    
    //send grams...
    sprintf(vBuf, "%3d", feedWeightVector.getItem(0));
    client.print(vBuf);
    Serial.print(vBuf);
    
    for(int i=1; i<gramCnt; i++) {
      sprintf(vBuf, ",%3d", feedWeightVector.getItem(i));
      client.print(vBuf);
      Serial.print(vBuf);
    }
    client.println();
    Serial.println();
    
    // note the time that the connection was made:
    Serial.println("done.");
    lastConnectionTime = millis();
    return true;
  } else {
    // if you couldn't make a connection:
    Serial.println("failure.");
    client.stop();
    return false;
  }
}

boolean httpRequest_getTime() {
  Serial.println("connecting...GetTime...");
  if (client.connect(server, 80)) {
    
    client.println("POST /hw/time/now/ HTTP/1.1");
    client.println("Host: www.wepet.net");
    client.println("User-Agent: WepetFeeder");
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
