void printGPS(float& flat, float& flon, unsigned long& age, TinyGPS& gps) {
  unsigned long chars;
  unsigned short sentences, failed;
  
  Serial.print("LAT=");
  Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  Serial.print(" LON=");
  Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
  Serial.print(" SAT=");
  Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
  Serial.print(" PREC=");
  Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
  
  gps.stats(&chars, &sentences, &failed);
  Serial.print(" CHARS=");
  Serial.print(chars);
  Serial.print(" SENTENCES=");
  Serial.print(sentences);
  Serial.print(" CSUM ERR=");
  Serial.println(failed);
    
}

float calcDistance(float lat1, float long1, float lat2, float long2) {
    /*
     위도,경도에 대한 절대값 계산
     */
    float lon = long1 > long2  ? (long1 - long2) : (long2-long1);
    float lat =  lat1 > lat2  ? (lat1 - lat2) : (lat2-lat1);
    
    /*
     경도에 대한 도분초및 거리 계산
     */
    int rad = (int)lon;
    int min = (int)((lon-rad)*60);
    float sec = ((lon-rad)*60 - min)*60;
    unsigned int lon_dist, lat_dist;
    lon_dist = ((rad * 88.8f) + (min*1.48f) + (sec*0.025f)) * 1000; // m단위
    
    /*
     위도에 대한 도분초및 거리 계산
     */
    rad = (int)lat;
    min = (int)((lat-rad)*60);
    sec = ((lat-rad)*60 - min)*60;
    lat_dist = ((rad * 111.f) + (min*1.85f) + (sec*0.0308f)) * 1000; // m단위
    
    float dist = sqrt( (lon_dist*lon_dist) + (lat_dist*lat_dist) );
    
    return dist;
}
