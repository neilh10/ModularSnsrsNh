#include <stdio.h>
#include <stdarg.h>
#include "LTC2941.h"
#include <Sodaq_DS3231.h>
#include <timeLib.h>

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
  #define Ser1 SerialUSB
#else
  #define Ser1 Serial
#endif

float coulomb = 0, mAh = 0, percent = 0;
const char compile_date[] = __DATE__ " " __TIME__;
const char file_name[] = __FILE__;

const unsigned long mSec_PST_Offset = 28800000; //8*60*60*1000
const unsigned long mSecInMinute    =    60000;
const unsigned long mSecInHour      =  3600000;
  uint32_t currentEpochSodaqTimePST_mS = 0;



//Fast uint To Ascii with field size
char const digit[] = "0123456789";
char* utoas(int i, char b[],int fill,char sz){
    char* p = b+sz;

    memset(b,fill,sz);
    *p = (char)'\0';
    do{ //Move back, inserting digits as u go
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    return b;
}
// Createa a fast debug TimeStamp
void printDebugTime(void) {
  unsigned long TimeNow_mS = millis();
  uint32_t millisecs;  //0-999
  char sBuff[5]; //Max Size 999
  tmElements_t tm;
  uint32_t time=TimeNow_mS/1000;

  //Be fast and accurate : from TimeLib.h 
  tm.Second = time % 60;
  time /= 60; // now it is minutes
  tm.Minute = time % 60;
  time /= 60; // now it is hours
  tm.Hour = time % 24;
  //time /= 24; // now it is days

  millisecs =TimeNow_mS-(tm.Hour*mSecInHour+tm.Minute*mSecInMinute+tm.Second*1000);
  if (millisecs>999) millisecs=999;

  Ser1.print(utoas(tm.Hour,sBuff,'0',2));
  Ser1.print(":");
  Ser1.print(utoas(tm.Minute,sBuff,'0',2));
  Ser1.print(":"); 
  Ser1.print(utoas(tm.Second,sBuff,'0',2)); 
  Ser1.print("."); 
  Ser1.print(utoas(millisecs,sBuff,'0',3)); 
  Ser1.print(" ");
}
#if 1
// This gets the current epoch time (unix time, ie, the number of seconds
// from January 1, 1970 00:00:00 UTC) and corrects it for the specified time zone
#define EPOCH_TIME_OFF 946684800
DateTime dtFromEpoch(uint32_t epochTime)
{
    DateTime dt(epochTime - EPOCH_TIME_OFF);
    return dt;
}

// This converts a date-time object into a ISO8601 formatted string
const int8_t _timeZone = 0;
String formatDateTime_ISO8601_dt(DateTime& dt)
{
    // Set up an inital string
    String dateTimeStr;
    // Convert the DateTime object to a String
    dt.addToString(dateTimeStr);
    dateTimeStr.replace(" ", "T");
    String tzString = String(_timeZone);
    if (-24 <= _timeZone && _timeZone <= -10)
    {
        tzString += F(":00");
    }
    else if (-10 < _timeZone && _timeZone < 0)
    {
        tzString = tzString.substring(0,1) + F("0") + tzString.substring(1,2) + F(":00");
    }
    else if (_timeZone == 0)
    {
        tzString = F("Z");
    }
    else if (0 < _timeZone && _timeZone < 10)
    {
        tzString = "+0" + tzString + F(":00");
    }
    else if (10 <= _timeZone && _timeZone <= 24)
    {
        tzString = "+" + tzString + F(":00");
    }
    dateTimeStr += tzString;
    return dateTimeStr;
}


// This converts an epoch time (unix time) into a ISO8601 formatted string
String formatDateTime_ISO8601_u32(uint32_t epochTime)
{
    // Create a DateTime object from the epochTime
    DateTime dt = dtFromEpoch(epochTime);
    return formatDateTime_ISO8601_dt(dt);
}
#endif//0
void setup(void)
{
    Wire.begin();
    
    Ser1.begin(115200);

    while(!Ser1){delay(10);};
    
    Ser1.println("---LTC2941 Readings");
    Ser1.println(compile_date);
    Ser1.println(file_name); //Dir and filename
    ltc2941.initialize();
    //ltc2941.setBatteryFullMAh(500,true);
}
static float coulomb_last=0;
static float mAh_last=0;
bool firstTime=false;
void loop(void)
{
    char sBuf[16];
    uint8_t reading_status;
    reading_status = 0x7F & ltc2941.getAccumulatedChargeStatus();
    coulomb = ltc2941.getCoulombs(false);
    mAh = ltc2941.getmAh(false);
    percent = ltc2941.getPercent(false);
    printDebugTime();
    #define LTC_STATUS_ACC 0x20
    if (reading_status & LTC_STATUS_ACC) {
        Ser1.print(F("Over/Under Status:"));
        Ser1.print(reading_status);         
         Ser1.print(F(", mAh:")); 
        Ser1.print(mAh,4);
        Ser1.println(F(" Reset ACC!")); 
        ltc2941.setAccumulatedCharge(LTC_ACC_CHARGE_DEF);
    } else {
        //Ser1.print("C:"); Ser1.print(dtostrf((coulomb_last-coulomb),8,4,sBuf));
        //Ser1.print("/");
        ///Ser1.print(coulomb,4);

        float mAh_diff = mAh_last-mAh;
        if (reading_status) 
        {
            Ser1.print(" Status:");
            Ser1.print(reading_status); 
        }
        //Ser1.print(" %:");
        //Ser1.print(percent);
        Ser1.print(" mAh:"); 
        //Ser1.print(dtostrf((mAh_diff),8,4,sBuf));
        //Ser1.print("/");
        Ser1.print(mAh,4);
        Ser1.print(", Diff mAS,");
#define cTimeSample_Sec 2
        Ser1.print(mAh_diff*60*60*cTimeSample_Sec);
    
        coulomb_last = coulomb;
        mAh_last=mAh;

        Ser1.println();
    }
    delay(cTimeSample_Sec*1000-10);
}