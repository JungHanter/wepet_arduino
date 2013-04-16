static const int MAXDAYS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

class Calendar
{
private:
  int year, month, day, hour, minute;
  boolean bLeep;
  
  boolean checkLeepYear() {
    if(year%4 == 0) {       //4의 배수 윤년인데
      if(year%100 == 0) {   //100의 배수 윤년아님
        if(year%400 == 0) { //400의 배수는 윤년!
          bLeep = true;     //400의 배수 윤년
        } else {
          bLeep = false;    //100의 배수 윤년아님
        }
      } else {
        bLeep = true;       //4의 배수 윤년
      }
    } else {
      bLeep = false;
    }
    
    return bLeep;
  }
  
public:
  Calendar() {
    year = 2013;
    month = 1;
    day = 1;
    hour = 0;
    minute = 0;
    bLeep = false;
  }
  
  Calendar(int year, int month, int day, int hour, int minute) {
    this->year = year;
    this->month = month;
    this->day = day;
    this->hour = hour;
    this->minute = minute;
    checkLeepYear();
  }
  
  int getYear() { return year; }
  byte getMonth() { return (byte)month; }
  byte getDay() { return (byte)day; }
  int getMMDD() { return (month*100)+day; }
  long getYYYYMMDD() { return ((long)(year)*10000)+((long)(month)*100)+(long)day; }
  
  byte getHour() { return (byte)hour; }
  byte getMinute() { return (byte)minute; }
  int getHHMM() { return (hour*100)+minute; }
  
  
  void addYear() {
    year++;
    checkLeepYear();
  }
  
  void addMonth() {
    month++;
    if(month == 13) {
      addYear();
      month = 1;
    }
  }
  
  void addDay() {
    day++;
    if( bLeep && (month==2) ) { //윤년의 2월(윤달)인 경우
      if(day > 29) {
        addMonth();
        day = 1;
      }
    } else {
      if( day > MAXDAYS[month-1] ) {
        addMonth();
        day = 1;
      }
    }
  }
  
  void addHour() {
    hour++;
    if(hour == 24) {
      addDay();
      hour = 0;
    }
  }
  
  void addMinute() {
    minute++;
    if(minute == 60) {
      addHour();
      minute = 0;
    }
  }
  
};
