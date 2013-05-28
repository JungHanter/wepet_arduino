void printGPS(float& flat, float& flon, unsigned short failed) {
  Serial.print("LAT=");
  Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 1000.0 : flat, 6);
  Serial.print(" / LON=");
  Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 1000.0 : flon, 6);    
  Serial.print(" / FAILED=");
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

boolean setSettings(const char* buf) {
  if(buf[0] == '0') {
    if(buf[2] == '0') {
      Serial.println("not registed SN!");
    } else {
      Serial.println("setting transmission failure.");
    }
    return false;
  }
  
  int what = 0;
  char* nowStr;
  char strLat[8], strLatF[8], strLon[8], strLonF[8], strDist[8];
  int bufCnt=0, cnt=0;
  
  bufCnt=2;
  nowStr = strLat;
  while(true) {
    if(buf[bufCnt] == ' ') {
      
    } else if(buf[bufCnt] == '\0' || buf[bufCnt] == '\n') {
      strDist[cnt] = '\0';
      break;
      
    } else if(buf[bufCnt] == '.' || buf[bufCnt] == ',') {
      what++;
      
      nowStr[cnt] = '\0';
      if(what == 0) {
        nowStr = strLat;
      } else if(what == 1) {
        nowStr = strLatF;
      } else if(what == 2) {
        nowStr = strLon;
      } else if(what == 3) {
        nowStr = strLonF;
      } else if(what == 4) {
        nowStr = strDist;
      }
      
      cnt = 0;
    } else {
      nowStr[cnt] = buf[bufCnt];
      
      cnt++;
    }
    
    bufCnt++;
  }
  
  if(strLat[0] == '-')
    homeLat  = (float)atoi(strLat) - ( (float)atol(strLatF) / (float)pow(10, strlen(strLatF)) );
  else
    homeLat  = (float)atoi(strLat) + ( (float)atol(strLatF) / (float)pow(10, strlen(strLatF)) );
    
  if(strLon[0] == '-')
    homeLong = (float)atoi(strLon) - ( (float)atol(strLonF) / (float)pow(10, strlen(strLonF)) );
  else
    homeLong = (float)atoi(strLon) + ( (float)atol(strLonF) / (float)pow(10, strlen(strLonF)) );
    
  safeDist = atoi(strDist);
  
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
      if(strCalendar[pos] == ',') {
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
      if(strCalendar[pos] == ',') {
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
      if(strCalendar[pos] == ',') {
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
      if(strCalendar[pos] == ',') {
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
