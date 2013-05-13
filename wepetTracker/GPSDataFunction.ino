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
    if(lat1 == TinyGPS::GPS_INVALID_F_ANGLE || long1 == TinyGPS::GPS_INVALID_F_ANGLE ||
       lat2 == TinyGPS::GPS_INVALID_F_ANGLE || long2 == TinyGPS::GPS_INVALID_F_ANGLE)
         return 0.0f;
  
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

void updateCalendar(const char* strCalendar) {
  int pos, chPos;
  boolean bFinish = false;

  int stat = 0; //stat 0(year) ~ 4(minute)
  char chYear[5], chMonth[3], chDay[3], chHour[3], chMinute[3];
  char* nowCh = chYear;

  pos = chPos = 0;

  while(true) {
    switch (stat) {
    case 0:     //year
      if(strCalendar[pos] == '/') {
        chYear[chPos] = '\0';

        stat = 1;
        nowCh = chMonth;
        chPos = 0;
      } 
      else {
        chYear[chPos++] = strCalendar[pos];
      }
      break;
    case 1:     //month
      if(strCalendar[pos] == '/') {
        chMonth[chPos] = '\0';

        stat = 2;
        nowCh = chDay;
        chPos = 0;
      } 
      else {
        chMonth[chPos++] = strCalendar[pos];
      }
      break;
    case 2:     //day
      if(strCalendar[pos] == ' ') {
        chDay[chPos] = '\0';

        stat = 3;
        nowCh = chHour;
        chPos = 0;
      } 
      else {
        chDay[chPos++] = strCalendar[pos];
      }
      break;
    case 3:     //hour
      if(strCalendar[pos] == ':') {
        chHour[chPos] = '\0';

        stat = 4;
        nowCh = chMinute;
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
      data.setCalendar(atoi(chYear), atoi(chMonth), atoi(chDay), atoi(chHour), atoi(chMinute));
      bDataUsing = false;
      return;
    }

    pos++;
  }
}
