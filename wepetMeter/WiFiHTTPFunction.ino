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

boolean httpRequest_sendData(const char* data, int dataSize) {
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...SendData...");

    client.println("POST /pedometer_raw_data/ HTTP/1.1");
    client.println("Host: rhinodream.com");
    client.println("User-Agent: Pedometer");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(dataSize);
    client.println();
    client.print(data);
    
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
    client.println("User-Agent: Pedometer");
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

