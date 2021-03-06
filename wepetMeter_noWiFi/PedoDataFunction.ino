
void printAllPedoData() {
  PedoData::Page page;
  int link = -1;

  do {
    while(bDataUsing);
    bDataUsing=true;
    link = data.readNextUsingPage(page);
    bDataUsing=false;

    if(link == -2) {
      Serial.println("Pedo data is empty.");
      return;
    } 
    else {
      Serial.print("*** print Page : ");
      Serial.print(page.pageNum);
      Serial.println(" ***");
      printPage(page);
      
      while(bDataUsing);
      bDataUsing=true;
      data.setPageUnused(page);
      bDataUsing=false;
    }

  } 
  while (link != -1);
}


void printPage(PedoData::Page& page) {
  char strBuf[20];
  int stepArr[6], stepNum;

  Serial.print("date : ");
  Serial.println(getPageDate(page, strBuf));

  Serial.print("time: ");
  Serial.println(getPageTime(page, strBuf));

  getPageSteps(page, stepArr, &stepNum);
  Serial.print("steps : [");
  for(int i=0; i<stepNum; i++) {
    if(i!=0) Serial.print(", ");
    Serial.print(stepArr[i]);
  } 
  Serial.println("]");

  Serial.print("actives : ");
  Serial.println(getPageActives(page));
  Serial.println();
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
      
      minutes = atoi(chMinute) -1;
      return;
    }

    pos++;
  }
}


const char* getPageDate(PedoData::Page& page, char* buf) {
  String strDate = String("");

  strDate += page.header.yyyy;
  strDate += ',';
  strDate += (int)((page.header.mmdd)/100);
  strDate += ',';
  strDate += (int)((page.header.mmdd)%100);

  strDate.toCharArray(buf, CHAR_BUF_SIZE);
  return buf;
}

const char* getPageTime(PedoData::Page& page, char* buf) {
  String strTime = String("");

  strTime += (int)((page.header.hhmm)/100);
  strTime += ',';
//  strTime += (int)((page.header.hhmm)%100);
  strTime += '0';

  strTime.toCharArray(buf, CHAR_BUF_SIZE);
  return buf;
}

void getPageSteps(PedoData::Page& page, int* arrStep, int* pCount) {
  *pCount = 0;
  for(int i=0; i<6; i++) {
    arrStep[i] = 0;
  }

  boolean bEnd = false;

  int startMinute = (page.header.hhmm)%100;
  int nowMinuteCount = startMinute%10;
  int nowSteps = 0;
  for(int i=0; i<page.header.recordCount; i++) {
    nowSteps += page.records[i].steps;

    nowMinuteCount++;
    if(nowMinuteCount == 10) {
      nowMinuteCount = 0;
      arrStep[(*pCount)] = nowSteps;
      (*pCount)++;
      nowSteps = 0;

      if(i == page.header.recordCount-1) {
        bEnd = true;
      }
    }
  }

  if(!bEnd) {
    arrStep[(*pCount)] = nowSteps;
    (*pCount)++;
  }
}

int getPageActives(PedoData::Page& page) {
  int actives = 0;

  for(int i=0; i<page.header.recordCount; i++) {
    actives += page.records[i].actives;
  }

  return actives;
}

